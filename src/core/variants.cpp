#include "variants.hpp"
#include <algorithm>
#include <utility>

#include <qcontainerfwd.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlengine.h>

#include "reload.hpp"

void Variants::onReload(QObject* oldInstance) {
	auto* old = qobject_cast<Variants*>(oldInstance);

	for (auto& [variant, instanceObj]: this->instances.values) {
		QObject* oldInstance = nullptr;
		if (old != nullptr) {
			auto& values = old->instances.values;

			int matchcount = 0;
			int matchi = 0;
			int i = 0;
			for (auto& [valueSet, _]: values) {
				int count = 0;
				for (auto& [k, v]: variant.toStdMap()) {
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
		}

		auto* instance = qobject_cast<Reloadable*>(instanceObj);

		if (instance != nullptr) instance->onReload(oldInstance);
		else Reloadable::reloadChildrenRecursive(instanceObj, oldInstance);
	}
}

void Variants::setVariants(QVariantList variants) {
	this->mVariants = std::move(variants);
	this->updateVariants();
}

void Variants::componentComplete() {
	this->Reloadable::componentComplete();
	this->updateVariants();
}

void Variants::updateVariants() {
	if (this->mComponent == nullptr) {
		qWarning() << "Variants instance does not have a component specified";
		return;
	}

	// clean up removed entries
	for (auto iter = this->instances.values.begin(); iter < this->instances.values.end();) {
		if (this->mVariants.contains(iter->first)) {
			iter++;
		} else {
			iter->second->deleteLater();
			iter = this->instances.values.erase(iter);
		}
	}

	for (auto iter = this->mVariants.begin(); iter < this->mVariants.end(); iter++) {
		auto& variantObj = *iter;
		if (!variantObj.canConvert<QVariantMap>()) {
			qWarning() << "value passed to Variants is not an object and will be ignored:" << variantObj;
		} else {
			auto variant = variantObj.value<QVariantMap>();

			for (auto iter2 = this->mVariants.begin(); iter2 < iter; iter2++) {
				if (*iter2 == variantObj) {
					qWarning() << "same value specified twice in Variants, duplicates will be ignored:"
					           << variantObj;
					goto outer;
				}
			}

			if (this->instances.contains(variant)) {
				continue; // we dont need to recreate this one
			}

			auto* instance = this->mComponent->createWithInitialProperties(
			    variant,
			    QQmlEngine::contextForObject(this->mComponent)
			);

			if (instance == nullptr) {
				qWarning() << this->mComponent->errorString().toStdString().c_str();
				qWarning() << "failed to create variant with object" << variant;
				continue;
			}

			instance->setParent(this);
			this->instances.insert(variant, instance);
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
void AwfulMap<K, V>::insert(K key, V value) {
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
