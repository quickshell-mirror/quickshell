#pragma once

#include <qabstractitemmodel.h>
#include <qcontainerfwd.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

///! QML model reflecting a javascript expression
/// ScriptModel is a QML [Data Model] that generates model operations based on changes
/// to a javascript expression attached to @@values.
///
/// ### When should I use this
/// ScriptModel should be used when you would otherwise use a javascript expression as a model,
/// [QAbstractItemModel] is accepted, and the data is likely to change over the lifetime of the program.
///
/// When directly using a javascript expression as a model, types like @@QtQuick.Repeater or @@QtQuick.ListView
/// will destroy all created delegates, and re-create the entire list. In the case of @@QtQuick.ListView this
/// will also prevent animations from working. If you wrap your expression with ScriptModel, only new items
/// will be created, and ListView animations will work as expected.
///
/// ### Example
/// ```qml
/// // Will cause all delegates to be re-created every time filterText changes.
/// @@QtQuick.Repeater {
///   model: myList.filter(entry => entry.name.startsWith(filterText))
///   delegate: // ...
/// }
///
/// // Will add and remove delegates only when required.
/// @@QtQuick.Repeater {
///   model: ScriptModel {
///     values: myList.filter(entry => entry.name.startsWith(filterText))
///   }
///
///   delegate: // ...
/// }
/// ```
class ScriptModel: public QAbstractListModel {
	Q_OBJECT;
	/// The list of values to reflect in the model.
	/// > [!WARNING] ScriptModel currently only works with lists of *unique* values.
	/// > There must not be any duplicates in the given list, or behavior of the model is undefined.
	///
	/// > [!TIP] @@ObjectModel$s supplied by Quickshell types will only contain unique values,
	/// > and can be used like so:
	/// >
	/// > ```qml
	/// > ScriptModel {
	/// >   values: DesktopEntries.applications.values.filter(...)
	/// > }
	/// > ```
	/// >
	/// > Note that we are using @@DesktopEntries.values because it will cause @@ScriptModel.values
	/// > to receive an update on change.
	Q_PROPERTY(QVariantList values READ values WRITE setValues NOTIFY valuesChanged);
	QML_ELEMENT;

public:
	[[nodiscard]] const QVariantList& values() const { return this->mValues; }
	void setValues(const QVariantList& newValues);

	[[nodiscard]] qint32 rowCount(const QModelIndex& parent) const override;
	[[nodiscard]] QVariant data(const QModelIndex& index, qint32 role) const override;
	[[nodiscard]] QHash<int, QByteArray> roleNames() const override;

signals:
	void valuesChanged();

private:
	QVariantList mValues;

	void updateValuesUnique(const QVariantList& newValues);
};
