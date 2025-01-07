#pragma once

#include <qcontainerfwd.h>
#include <qlist.h>
#include <qlogging.h>
#include <qmap.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmllist.h>
#include <qqmlparserstatus.h>
#include <qtmetamacros.h>
#include <qvariant.h>

#include "doc.hpp"
#include "reload.hpp"

// extremely inefficient map
template <typename K, typename V>
class AwfulMap {
public:
	[[nodiscard]] bool contains(const K& key) const;
	[[nodiscard]] V* get(const K& key);
	void insert(const K& key, V value); // assumes no duplicates
	bool remove(const K& key);          // returns true if anything was removed
	QList<QPair<K, V>> values;
};

///! Creates instances of a component based on a given model.
/// Creates and destroys instances of the given component when the given property changes.
///
/// `Variants` is similar to @@QtQuick.Repeater except it is for *non @@QtQuick.Item$* objects, and acts as
/// a reload scope.
///
/// Each non duplicate value passed to @@model will create a new instance of
/// @@delegate with a `modelData` property set to that value.
///
/// See @@Quickshell.screens for an example of using `Variants` to create copies of a window per
/// screen.
///
/// > [!WARNING] BUG: Variants currently fails to reload children if the variant set is changed as
/// > it is instantiated. (usually due to a mutation during variant creation)
class Variants: public Reloadable {
	Q_OBJECT;
	/// The component to create instances of.
	///
	/// The delegate should define a `modelData` property that will be popuplated with a value
	/// from the @@model.
	Q_PROPERTY(QQmlComponent* delegate MEMBER mDelegate);
	/// The list of sets of properties to create instances with.
	/// Each set creates an instance of the component, which are updated when the input sets update.
	QSDOC_PROPERTY_OVERRIDE(QList<QVariant> model READ model WRITE setModel NOTIFY modelChanged);
	QSDOC_HIDE Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged);
	/// Current instances of the delegate.
	Q_PROPERTY(QQmlListProperty<QObject> instances READ instances NOTIFY instancesChanged);
	Q_CLASSINFO("DefaultProperty", "delegate");
	QML_ELEMENT;

public:
	explicit Variants(QObject* parent = nullptr): Reloadable(parent) {}

	void onReload(QObject* oldInstance) override;
	void componentComplete() override;

	[[nodiscard]] QVariant model() const;
	void setModel(const QVariant& model);

	QQmlListProperty<QObject> instances();

signals:
	void modelChanged();
	void instancesChanged();

private:
	static qsizetype instanceCount(QQmlListProperty<QObject>* prop);
	static QObject* instanceAt(QQmlListProperty<QObject>* prop, qsizetype i);

	void updateVariants();

	QQmlComponent* mDelegate = nullptr;
	QVariantList mModel;
	AwfulMap<QVariant, QObject*> mInstances;
	bool loaded = false;
};
