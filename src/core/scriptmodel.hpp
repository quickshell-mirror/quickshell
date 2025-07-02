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
/// [QAbstractItemModel]: https://doc.qt.io/qt-6/qabstractitemmodel.html
/// [Data Model]: https://doc.qt.io/qt-6/qtquick-modelviewsdata-modelview.html#qml-data-models
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
	/// > Note that we are using @@ObjectModel.values because it will cause @@ScriptModel.values
	/// > to receive an update on change.
	///
	/// > [!TIP] Most lists exposed by Quickshell are read-only. Some operations like `sort()`
	/// > act on a list in-place and cannot be used directly on a list exposed by Quickshell.
	/// > You can copy a list using spread syntax: `[...variable]` instead of `variable`.
	/// >
	/// > For example:
	/// > ```qml
	/// > ScriptModel {
	/// >   values: [...DesktopEntries.applications.values].sort(...)
	/// > }
	/// > ```
	Q_PROPERTY(QVariantList values READ values WRITE setValues NOTIFY valuesChanged);
	/// The property that javascript objects passed into the model will be compared with.
	///
	/// For example, if `objectProp` is `"myprop"` then `{ myprop: "a", other: "y" }` and
	/// `{ myprop: "a", other: "z" }` will be considered equal.
	///
	/// Defaults to `""`, meaning no key.
	Q_PROPERTY(QString objectProp READ objectProp WRITE setObjectProp NOTIFY objectPropChanged);
	QML_ELEMENT;

public:
	[[nodiscard]] QVariantList values() const {
		auto values = this->mValues;
		// If not detached, the QML engine will invalidate iterators in updateValuesUnique.
		if (this->hasActiveIterators) values.detach();
		return values;
	}

	void setValues(const QVariantList& newValues);

	[[nodiscard]] QString objectProp() const { return this->cmpKey; }
	void setObjectProp(const QString& objectProp);

	[[nodiscard]] qint32 rowCount(const QModelIndex& parent) const override;
	[[nodiscard]] QVariant data(const QModelIndex& index, qint32 role) const override;
	[[nodiscard]] QHash<int, QByteArray> roleNames() const override;

signals:
	void valuesChanged();
	void objectPropChanged();

private:
	QVariantList mValues;
	QString cmpKey;
	bool hasActiveIterators = false;

	void updateValuesUnique(const QVariantList& newValues);
};
