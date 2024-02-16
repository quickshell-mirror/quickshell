#pragma once

#include <qcontainerfwd.h>
#include <qlist.h>
#include <qlogging.h>
#include <qmap.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlparserstatus.h>
#include <qtmetamacros.h>

#include "reload.hpp"

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

///! Creates instances of a component based on a given set of variants.
/// Creates and destroys instances of the given component when the given property changes.
///
/// See [QuickShell.screens] for an example of using `Variants` to create copies of a window per
/// screen.
///
/// [QuickShell.screens]: ../quickshell#prop.screens
class Variants: public Reloadable {
	Q_OBJECT;
	/// The component to create instances of
	Q_PROPERTY(QQmlComponent* component MEMBER mComponent);
	/// The list of sets of properties to create instances with.
	/// Each set creates an instance of the component, which are updated when the input sets update.
	Q_PROPERTY(QList<QVariant> variants MEMBER mVariants WRITE setVariants);
	Q_CLASSINFO("DefaultProperty", "component");
	QML_ELEMENT;

public:
	explicit Variants(QObject* parent = nullptr): Reloadable(parent) {}

	void onReload(QObject* oldInstance) override;

	void componentComplete() override;

private:
	void setVariants(QVariantList variants);
	void updateVariants();

	QQmlComponent* mComponent = nullptr;
	QVariantList mVariants;
	AwfulMap<QVariantMap, QObject*> instances;
};
