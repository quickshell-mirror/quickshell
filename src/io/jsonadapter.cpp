#include "jsonadapter.hpp"

#include <qcontainerfwd.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qjsvalue.h>
#include <qmetaobject.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qqml.h>
#include <qqmlengine.h>
#include <qqmlinfo.h>
#include <qqmllist.h>
#include <qstringview.h>
#include <qvariant.h>

namespace qs::io {

void JsonAdapter::componentComplete() { this->connectNotifiers(); }

void JsonAdapter::deserializeAdapter(const QByteArray& data) {
	if (data.isEmpty()) return;

	// Importing this makes CI builds fail for some reason.
	QJsonParseError error; // NOLINT (misc-include-cleaner)
	auto json = QJsonDocument::fromJson(data, &error);

	if (error.error != QJsonParseError::NoError) {
		qmlWarning(this) << "Failed to deserialize json: " << error.errorString();
		return;
	}

	if (!json.isObject()) {
		qmlWarning(this) << "Failed to deserialize json: not an object";
		return;
	}

	this->changesBlocked = true;
	this->oldCreatedObjects = this->createdObjects;
	this->createdObjects.clear();

	this->deserializeRec(json.object(), this, &JsonAdapter::staticMetaObject);

	for (auto* object: oldCreatedObjects) {
		delete object; // FIXME: QMetaType::destroy?
	}

	this->oldCreatedObjects.clear();
	this->changesBlocked = false;

	this->connectNotifiers();
}

void JsonAdapter::connectNotifiers() {
	auto notifySlot = JsonAdapter::staticMetaObject.indexOfSlot("onPropertyChanged()");
	connectNotifiersRec(notifySlot, this, &JsonAdapter::staticMetaObject);
}

void JsonAdapter::connectNotifiersRec(int notifySlot, QObject* obj, const QMetaObject* base) {
	const auto* metaObject = obj->metaObject();

	for (auto i = base->propertyOffset(); i != metaObject->propertyCount(); i++) {
		const auto prop = metaObject->property(i);

		if (prop.isReadable() && prop.hasNotifySignal()) {
			QMetaObject::connect(obj, prop.notifySignalIndex(), this, notifySlot, Qt::UniqueConnection);

			auto val = prop.read(obj);
			if (val.canView<JsonObject*>()) {
				auto* pobj = prop.read(obj).view<JsonObject*>();
				if (pobj) connectNotifiersRec(notifySlot, pobj, &JsonObject::staticMetaObject);
			} else if (val.canConvert<QQmlListProperty<JsonObject>>()) {
				auto listVal = val.value<QQmlListProperty<JsonObject>>();

				auto len = listVal.count(&listVal);
				for (auto i = 0; i != len; i++) {
					auto* pobj = listVal.at(&listVal, i);

					if (pobj) connectNotifiersRec(notifySlot, pobj, &JsonObject::staticMetaObject);
				}
			}
		}
	}
}

void JsonAdapter::onPropertyChanged() {
	if (this->changesBlocked) return;

	this->connectNotifiers();
	this->adapterUpdated();
}

QByteArray JsonAdapter::serializeAdapter() {
	return QJsonDocument(this->serializeRec(this, &JsonAdapter::staticMetaObject))
	    .toJson(QJsonDocument::Indented);
}

QJsonObject JsonAdapter::serializeRec(const QObject* obj, const QMetaObject* base) const {
	QJsonObject json;
	const auto* metaObject = obj->metaObject();

	for (auto i = base->propertyOffset(); i != metaObject->propertyCount(); i++) {
		const auto prop = metaObject->property(i);

		if (prop.isReadable() && prop.hasNotifySignal()) {
			auto val = prop.read(obj);
			if (val.canView<JsonObject*>()) {
				auto* pobj = val.view<JsonObject*>();

				if (pobj) {
					json.insert(prop.name(), serializeRec(pobj, &JsonObject::staticMetaObject));
				} else {
					json.insert(prop.name(), QJsonValue::Null);
				}
			} else if (val.canConvert<QQmlListProperty<JsonObject>>()) {
				QJsonArray array;
				auto listVal = val.value<QQmlListProperty<JsonObject>>();

				auto len = listVal.count(&listVal);
				for (auto i = 0; i != len; i++) {
					auto* pobj = listVal.at(&listVal, i);

					if (pobj) {
						array.push_back(serializeRec(pobj, &JsonObject::staticMetaObject));
					} else {
						array.push_back(QJsonValue::Null);
					}
				}

				json.insert(prop.name(), array);
			} else if (val.canConvert<QJSValue>()) {
				auto variant = val.value<QJSValue>().toVariant();
				auto jv = QJsonValue::fromVariant(variant);
				json.insert(prop.name(), jv);
			} else {
				auto jv = QJsonValue::fromVariant(val);
				json.insert(prop.name(), jv);
			}
		}
	}

	return json;
}

void JsonAdapter::deserializeRec(const QJsonObject& json, QObject* obj, const QMetaObject* base) {
	const auto* metaObject = obj->metaObject();

	for (auto i = base->propertyOffset(); i != metaObject->propertyCount(); i++) {
		const auto prop = metaObject->property(i);
		if (json.contains(prop.name())) {
			auto jval = json.value(prop.name());

			if (prop.metaType() == QMetaType::fromType<QVariant>()) {
				auto variant = jval.toVariant();
				auto oldValue = prop.read(this).value<QJSValue>();

				// Calling prop.write with a new QJSValue will cause a property update
				// even if content is identical.
				if (jval.toVariant() != oldValue.toVariant()) {
					auto jsValue = qmlEngine(this)->fromVariant<QJSValue>(jval.toVariant());
					prop.write(this, QVariant::fromValue(jsValue));
				}
			} else if (QMetaType::canView(prop.metaType(), QMetaType::fromType<JsonObject*>())) {
				// FIXME: This doesn't support creating descendants of JsonObject, as QMetaType.metaObject()
				// returns null for QML types.

				if (jval.isObject()) {
					auto* currentValue = prop.read(obj).view<JsonObject*>();
					auto isNew = currentValue == nullptr;

					if (isNew) {
						// metaObject->metaType removes the pointer
						currentValue =
						    static_cast<JsonObject*>(prop.metaType().metaObject()->metaType().create());

						currentValue->setParent(this);
						this->createdObjects.push_back(currentValue);
					} else if (oldCreatedObjects.removeOne(currentValue)) {
						createdObjects.push_back(currentValue);
					}

					this->deserializeRec(jval.toObject(), currentValue, &JsonObject::staticMetaObject);

					if (isNew) prop.write(obj, QVariant::fromValue(currentValue));
				} else if (jval.isNull()) {
					prop.write(obj, QVariant::fromValue(nullptr));
				} else {
					qmlWarning(this) << "Failed to deserialize property " << prop.name() << " as object. Got "
					                 << jval.toVariant().typeName();
				}
			} else if (QMetaType::canConvert(
			               prop.metaType(),
			               QMetaType::fromType<QQmlListProperty<JsonObject>>()
			           ))
			{
				auto pval = prop.read(this);

				if (pval.canConvert<QQmlListProperty<JsonObject>>()) {
					auto lp = pval.value<QQmlListProperty<JsonObject>>();
					auto array = jval.toArray();
					auto lpCount = lp.count(&lp);

					auto i = 0;
					for (; i != array.count(); i++) {
						JsonObject* currentValue = nullptr;
						auto isNew = i >= lpCount;

						const auto& jsonValue = array.at(i);
						if (jsonValue.isObject()) {
							if (isNew) {
								currentValue = lp.at(&lp, i);
								if (oldCreatedObjects.removeOne(currentValue)) {
									createdObjects.push_back(currentValue);
								}
							} else {
								// FIXME: should be the type inside the QQmlListProperty but how can we get that?
								currentValue = static_cast<JsonObject*>(QMetaType::fromType<JsonObject>().create());
								currentValue->setParent(this);
								this->createdObjects.push_back(currentValue);
							}

							this->deserializeRec(
							    jsonValue.toObject(),
							    currentValue,
							    &JsonObject::staticMetaObject
							);
						} else if (!jsonValue.isNull()) {
							qmlWarning(this) << "Failed to deserialize property" << prop.name()
							                 << ": Member of object array is not an object: "
							                 << jsonValue.toVariant().typeName();
						}

						if (isNew) {
							lp.append(&lp, currentValue);
						}
					}

					for (; i < lpCount; i++) {
						lp.removeLast(&lp);
					}
				} else {
					qmlWarning(this) << "Failed to deserialize property " << prop.name()
					                 << ": property is a list<JsonObject> but contains null.";
				}
			} else {
				auto variant = jval.toVariant();

				if (variant.convert(prop.metaType())) {
					prop.write(obj, variant);
				} else {
					qmlWarning(this) << "Failed to deserialize property " << prop.name() << ": expected "
					                 << prop.metaType().name() << " but got " << jval.toVariant().typeName();
				}
			}
		}
	}
}

} // namespace qs::io
