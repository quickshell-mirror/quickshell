#pragma once

#include <qcontainerfwd.h>
#include <qlist.h>
#include <qmap.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlparserstatus.h>

// extremely inefficient map
template <typename K, typename V>
class AwfulMap {
public:
	[[nodiscard]] bool contains(const K& key) const;
	void insert(K key, V value); // assumes no duplicates
	bool remove(const K& key);   // returns true if anything was removed
	QList<QPair<K, V>> values;
};

class Variants: public QObject, public QQmlParserStatus {
	Q_OBJECT;
	Q_PROPERTY(QQmlComponent* component MEMBER mComponent);
	Q_PROPERTY(QVariantList variants MEMBER mVariants WRITE setVariants);
	Q_CLASSINFO("DefaultProperty", "component");
	QML_ELEMENT;

public:
	explicit Variants(QObject* parent = nullptr): QObject(parent) {}

	void classBegin() override {};
	void componentComplete() override;

private:
	void setVariants(QVariantList variants);
	void updateVariants();

	QQmlComponent* mComponent = nullptr;
	QVariantList mVariants;
	AwfulMap<QVariantMap, QObject*> instances;
};
