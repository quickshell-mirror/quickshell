#include "variants.hpp"
#include <algorithm>
#include <utility>

#include <qcontainerfwd.h>
#include <qlogging.h>

void Variants::setVariants(QVariantList variants) {
	this->mVariants = std::move(variants);
	qDebug() << "configurations updated:" << this->mVariants;

	this->updateVariants();
}

void Variants::componentComplete() {
	qDebug() << "configure ready";

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

			auto* instance = this->mComponent->createWithInitialProperties(variant, nullptr);

			if (instance == nullptr) {
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
