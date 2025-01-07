#include "variants.hpp"
#include <algorithm>
#include <utility>

#include <qcontainerfwd.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlengine.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "reload.hpp"

void Variants::onReload(QObject* oldInstance) {
	auto* old = qobject_cast<Variants*>(oldInstance);

	for (auto& [variant, instanceObj]: this->mInstances.values) {
		QObject* oldInstance = nullptr;
		if (old != nullptr) {
			auto& values = old->mInstances.values;

			if (variant.canConvert<QVariantMap>()) {
				auto variantMap = variant.value<QVariantMap>();

				int matchcount = 0;
				int matchi = 0;
				int i = 0;
				for (auto& [value, _]: values) {
					if (!value.canConvert<QVariantMap>()) continue;
					auto valueSet = value.value<QVariantMap>();

					int count = 0;
					for (auto [k, v]: variantMap.asKeyValueRange()) {
						if (valueSet.contains(k) && valueSet.value(k) == v) {
							count++;
						}
					}

					if (count > matchcount) {
						matchcount = count;
						matchi = i;
					}

					i++;
				}

				if (matchcount > 0) {
					oldInstance = values.takeAt(matchi).second;
				}
			} else {
				int i = 0;
				for (auto& [value, _]: values) {
					if (variant == value) {
						oldInstance = values.takeAt(i).second;
						break;
					}

					i++;
				}
			}
		}

		auto* instance = qobject_cast<Reloadable*>(instanceObj);

		if (instance != nullptr) instance->reload(oldInstance);
		else Reloadable::reloadChildrenRecursive(instanceObj, oldInstance);
	}

	this->loaded = true;
}

QVariant Variants::model() const { return QVariant::fromValue(this->mModel); }

void Variants::setModel(const QVariant& model) {
	if (model.canConvert<QVariantList>()) {
		this->mModel = model.value<QVariantList>();
	} else if (model.canConvert<QQmlListReference>()) {
		auto list = model.value<QQmlListReference>();
		if (!list.isReadable()) {
			qWarning() << "Non readable list" << model << "assigned to Variants.model, Ignoring.";
			return;
		}

		QVariantList model;
		auto size = list.count();
		for (auto i = 0; i < size; i++) {
			model.push_back(QVariant::fromValue(list.at(i)));
		}

		this->mModel = std::move(model);
	} else {
		qWarning() << "Non list data" << model << "assigned to Variants.model, Ignoring.";
		return;
	}

	this->updateVariants();
	emit this->modelChanged();
	emit this->instancesChanged();
}

QQmlListProperty<QObject> Variants::instances() {
	return QQmlListProperty<QObject>(this, nullptr, &Variants::instanceCount, &Variants::instanceAt);
}

qsizetype Variants::instanceCount(QQmlListProperty<QObject>* prop) {
	return static_cast<Variants*>(prop->object)->mInstances.values.length(); // NOLINT
}

QObject* Variants::instanceAt(QQmlListProperty<QObject>* prop, qsizetype i) {
	return static_cast<Variants*>(prop->object)->mInstances.values.at(i).second; // NOLINT
}

void Variants::componentComplete() {
	this->Reloadable::componentComplete();
	this->updateVariants();
}

void Variants::updateVariants() {
	if (this->mDelegate == nullptr) {
		qWarning() << "Variants instance does not have a component specified";
		return;
	}

	// clean up removed entries
	for (auto iter = this->mInstances.values.begin(); iter < this->mInstances.values.end();) {
		if (this->mModel.contains(iter->first)) {
			iter++;
		} else {
			iter->second->deleteLater();
			iter = this->mInstances.values.erase(iter);
		}
	}

	for (auto iter = this->mModel.begin(); iter < this->mModel.end(); iter++) {
		auto& variant = *iter;
		for (auto iter2 = this->mModel.begin(); iter2 < iter; iter2++) {
			if (*iter2 == variant) {
				qWarning() << "same value specified twice in Variants, duplicates will be ignored:"
				           << variant;
				goto outer;
			}
		}

		{
			if (this->mInstances.contains(variant)) {
				continue; // we dont need to recreate this one
			}

			auto variantMap = QVariantMap();
			variantMap.insert("modelData", variant);

			auto* instance = this->mDelegate->createWithInitialProperties(
			    variantMap,
			    QQmlEngine::contextForObject(this->mDelegate)
			);

			if (instance == nullptr) {
				qWarning() << this->mDelegate->errorString().toStdString().c_str();
				qWarning() << "failed to create variant with object" << variant;
				continue;
			}

			QQmlEngine::setObjectOwnership(instance, QQmlEngine::CppOwnership);

			instance->setParent(this);
			this->mInstances.insert(variant, instance);

			if (this->loaded) {
				if (auto* reloadable = qobject_cast<Reloadable*>(instance)) reloadable->reload(nullptr);
				else Reloadable::reloadChildrenRecursive(instance, nullptr);
			}
		}

	outer:;
	}
}

template <typename K, typename V>
bool AwfulMap<K, V>::contains(const K& key) const {
	return std::ranges::any_of(this->values, [&](const QPair<K, V>& pair) {
		return pair.first == key;
	});
}

template <typename K, typename V>
V* AwfulMap<K, V>::get(const K& key) {
	for (auto& [k, v]: this->values) {
		if (key == k) {
			return &v;
		}
	}

	return nullptr;
}

template <typename K, typename V>
void AwfulMap<K, V>::insert(const K& key, V value) {
	this->values.push_back(QPair<K, V>(key, value));
}

template <typename K, typename V>
bool AwfulMap<K, V>::remove(const K& key) {
	for (auto iter = this->values.begin(); iter < this->values.end(); iter++) {
		if (iter->first == key) {
			this->values.erase(iter);

			return true;
		}
	}

	return false;
}
