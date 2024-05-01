#include "boundcomponent.hpp"
#include <utility>

#include <qcontainerfwd.h>
#include <qlogging.h>
#include <qmetaobject.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qqmlcomponent.h>
#include <qqmlcontext.h>
#include <qqmlengine.h>
#include <qqmlerror.h>
#include <qquickitem.h>
#include <qtmetamacros.h>

#include "incubator.hpp"

QObject* BoundComponent::item() const { return this->object; }
QQmlComponent* BoundComponent::sourceComponent() const { return this->mComponent; }

void BoundComponent::setSourceComponent(QQmlComponent* component) {
	if (component == this->mComponent) return;

	if (this->componentCompleted) {
		qWarning() << "BoundComponent.component cannot be set after creation";
		return;
	}
	this->disconnectComponent();

	this->ownsComponent = false;
	this->mComponent = component;
	if (component != nullptr) {
		QObject::connect(component, &QObject::destroyed, this, &BoundComponent::onComponentDestroyed);
	}

	emit this->sourceComponentChanged();
}

void BoundComponent::disconnectComponent() {
	if (this->mComponent == nullptr) return;

	if (this->ownsComponent) {
		delete this->mComponent;
	} else {
		QObject::disconnect(this->mComponent, nullptr, this, nullptr);
	}

	this->mComponent = nullptr;
}

void BoundComponent::onComponentDestroyed() { this->mComponent = nullptr; }
QString BoundComponent::source() const { return this->mSource; }

void BoundComponent::setSource(QString source) {
	if (source == this->mSource) return;

	if (this->componentCompleted) {
		qWarning() << "BoundComponent.url cannot be set after creation";
		return;
	}

	auto* context = QQmlEngine::contextForObject(this);
	auto* component = new QQmlComponent(context->engine(), context->resolvedUrl(source), this);

	if (component->isError()) {
		qWarning() << component->errorString().toStdString().c_str();
		delete component;
	} else {
		this->disconnectComponent();
		this->ownsComponent = true;
		this->mSource = std::move(source);
		this->mComponent = component;

		emit this->sourceChanged();
		emit this->sourceComponentChanged();
	}
}

bool BoundComponent::bindValues() const { return this->mBindValues; }

void BoundComponent::setBindValues(bool bindValues) {
	if (this->componentCompleted) {
		qWarning() << "BoundComponent.bindValues cannot be set after creation";
		return;
	}

	this->mBindValues = bindValues;
	emit this->bindValuesChanged();
}

void BoundComponent::componentComplete() {
	this->QQuickItem::componentComplete();
	this->componentCompleted = true;
	this->tryCreate();
}

void BoundComponent::tryCreate() {
	if (this->mComponent == nullptr) {
		qWarning() << "BoundComponent has no component";
		return;
	}

	auto initialProperties = QVariantMap();

	const auto* metaObject = this->metaObject();
	for (auto i = metaObject->propertyOffset(); i < metaObject->propertyCount(); i++) {
		const auto prop = metaObject->property(i);

		if (prop.isReadable()) {
			initialProperties.insert(prop.name(), prop.read(this));
		}
	}

	this->incubator = new QsQmlIncubator(QsQmlIncubator::AsynchronousIfNested, this);
	this->incubator->setInitialProperties(initialProperties);

	// clang-format off
	QObject::connect(this->incubator, &QsQmlIncubator::completed, this, &BoundComponent::onIncubationCompleted);
	QObject::connect(this->incubator, &QsQmlIncubator::failed, this, &BoundComponent::onIncubationFailed);
	// clang-format on

	this->mComponent->create(*this->incubator, QQmlEngine::contextForObject(this));
}

void BoundComponent::onIncubationCompleted() {
	this->object = this->incubator->object();
	delete this->incubator;
	this->disconnectComponent();

	this->object->setParent(this);
	this->mItem = qobject_cast<QQuickItem*>(this->object);

	const auto* metaObject = this->metaObject();
	const auto* objectMetaObject = this->object->metaObject();

	if (this->mBindValues) {
		for (auto i = metaObject->propertyOffset(); i < metaObject->propertyCount(); i++) {
			const auto prop = metaObject->property(i);

			if (prop.isReadable() && prop.hasNotifySignal()) {
				const auto objectPropIndex = objectMetaObject->indexOfProperty(prop.name());

				if (objectPropIndex == -1) {
					qWarning() << "property" << prop.name()
					           << "defined on BoundComponent but not on its contained object.";
					continue;
				}

				const auto objectProp = objectMetaObject->property(objectPropIndex);
				if (objectProp.isWritable()) {
					auto* proxy = new BoundComponentPropertyProxy(this, this->object, prop, objectProp);
					proxy->onNotified(); // any changes that might've happened before connection
				} else {
					qWarning() << "property" << prop.name()
					           << "defined on BoundComponent is not writable for its contained object.";
				}
			}
		}
	}

	for (auto i = metaObject->methodOffset(); i < metaObject->methodCount(); i++) {
		const auto method = metaObject->method(i);

		if (method.name().startsWith("on") && method.name().length() > 2) {
			auto sig = QString(method.methodSignature()).sliced(2);
			if (!sig[0].isUpper()) continue;
			sig[0] = sig[0].toLower();
			auto name = sig.sliced(0, sig.indexOf('('));

			auto mostViableSignal = QMetaMethod();
			for (auto i = 0; i < objectMetaObject->methodCount(); i++) {
				const auto method = objectMetaObject->method(i);
				if (method.methodSignature() == sig) {
					mostViableSignal = method;
					break;
				}

				if (method.name() == name) {
					if (mostViableSignal.isValid()) {
						qWarning() << "Multiple candidates, so none will be attached for signal" << name;
						goto next;
					}

					mostViableSignal = method;
				}
			}

			if (!mostViableSignal.isValid()) {
				qWarning() << "Function" << method.name() << "appears to be a signal handler for" << name
				           << "but it does not match any signals on the target object";
				goto next;
			}

			QMetaObject::connect(
			    this->object,
			    mostViableSignal.methodIndex(),
			    this,
			    method.methodIndex()
			);
		}

	next:;
	}

	if (this->mItem != nullptr) {
		this->mItem->setParentItem(this);

		// clang-format off
		QObject::connect(this, &QQuickItem::widthChanged, this, &BoundComponent::updateSize);
		QObject::connect(this, &QQuickItem::heightChanged, this, &BoundComponent::updateSize);
		QObject::connect(this->mItem, &QQuickItem::implicitWidthChanged, this, &BoundComponent::updateImplicitSize);
		QObject::connect(this->mItem, &QQuickItem::implicitHeightChanged, this, &BoundComponent::updateImplicitSize);
		// clang-format on

		this->updateImplicitSize();
		this->updateSize();
	}

	emit this->loaded();
}

void BoundComponent::onIncubationFailed() {
	qWarning() << "Failed to create BoundComponent";

	for (auto& error: this->incubator->errors()) {
		qWarning() << error;
	}

	delete this->incubator;
	this->disconnectComponent();
}

void BoundComponent::updateSize() { this->mItem->setSize(this->size()); }

void BoundComponent::updateImplicitSize() {
	this->setImplicitWidth(this->mItem->implicitWidth());
	this->setImplicitHeight(this->mItem->implicitHeight());
}

BoundComponentPropertyProxy::BoundComponentPropertyProxy(
    QObject* from,
    QObject* to,
    QMetaProperty fromProperty,
    QMetaProperty toProperty
)
    : QObject(from)
    , from(from)
    , to(to)
    , fromProperty(fromProperty)
    , toProperty(toProperty) {
	const auto* metaObject = this->metaObject();
	auto method = metaObject->indexOfSlot("onNotified()");
	QMetaObject::connect(from, fromProperty.notifySignal().methodIndex(), this, method);
}

void BoundComponentPropertyProxy::onNotified() {
	this->toProperty.write(this->to, this->fromProperty.read(this->from));
}
