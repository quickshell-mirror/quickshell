#pragma once

#include <qcontainerfwd.h>
#include <qlist.h>
#include <qlogging.h>
#include <qmap.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlparserstatus.h>

#include "scavenge.hpp"

// extremely inefficient map
template <typename K, typename V>
class AwfulMap {
public:
	[[nodiscard]] bool contains(const K& key) const;
	[[nodiscard]] V* get(const K& key);
	void insert(K key, V value); // assumes no duplicates
	bool remove(const K& key);   // returns true if anything was removed
	QList<QPair<K, V>> values;
};

class Variants: public Scavenger, virtual public Scavengeable {
	Q_OBJECT;
	Q_PROPERTY(QQmlComponent* component MEMBER mComponent);
	Q_PROPERTY(QVariantList variants MEMBER mVariants WRITE setVariants);
	Q_CLASSINFO("DefaultProperty", "component");
	QML_ELEMENT;

public:
	explicit Variants(QObject* parent = nullptr): Scavenger(parent) {}

	void earlyInit(QObject* old) override;
	QObject* scavengeTargetFor(QObject* child) override;

	void componentComplete() override;

private:
	void setVariants(QVariantList variants);
	void updateVariants();

	QQmlComponent* mComponent = nullptr;
	QVariantList mVariants;
	AwfulMap<QVariantMap, QObject*> instances;

	// pointers may die post componentComplete.
	AwfulMap<QVariantMap, QObject*> scavengeableInstances;
	QVariantMap* activeScavengeVariant = nullptr;
};
