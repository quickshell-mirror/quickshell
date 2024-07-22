#pragma once

#include <qabstractitemmodel.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "model.hpp"

///! A Repeater / for loop / map for non Item derived objects.
/// > [!ERROR] Removed in favor of @@QtQml.Models.Instantiator
///
/// The ObjectRepeater creates instances of the provided delegate for every entry in the
/// given model, similarly to a @@QtQuick.Repeater but for non visual types.
class ObjectRepeater: public ObjectModel<QObject> {
	Q_OBJECT;
	/// The model providing data to the ObjectRepeater.
	///
	/// Currently accepted model types are `list<T>` lists, javascript arrays,
	/// and [QAbstractListModel] derived models, though only one column will be repeated
	/// from the latter.
	///
	/// Note: @@ObjectModel is a [QAbstractListModel] with a single column.
	///
	/// [QAbstractListModel]: https://doc.qt.io/qt-6/qabstractlistmodel.html
	Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged);
	/// The delegate component to repeat.
	///
	/// The delegate is given the same properties as in a Repeater, except `index` which
	/// is not currently implemented.
	///
	/// If the model is a `list<T>` or javascript array, a `modelData` property will be
	/// exposed containing the entry from the model. If the model is a [QAbstractListModel],
	/// the roles from the model will be exposed.
	///
	/// Note: @@ObjectModel has a single role named `modelData` for compatibility with normal lists.
	///
	/// [QAbstractListModel]: https://doc.qt.io/qt-6/qabstractlistmodel.html
	Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged);
	Q_CLASSINFO("DefaultProperty", "delegate");
	QML_ELEMENT;
	QML_UNCREATABLE("ObjectRepeater has been removed in favor of QtQml.Models.Instantiator.");

public:
	explicit ObjectRepeater(QObject* parent = nullptr): ObjectModel(parent) {}

	[[nodiscard]] QVariant model() const;
	void setModel(QVariant model);

	[[nodiscard]] QQmlComponent* delegate() const;
	void setDelegate(QQmlComponent* delegate);

signals:
	void modelChanged();
	void delegateChanged();

private slots:
	void onDelegateDestroyed();
	void onModelDestroyed();
	void onModelRowsInserted(const QModelIndex& parent, int first, int last);
	void onModelRowsRemoved(const QModelIndex& parent, int first, int last);

	void onModelRowsMoved(
	    const QModelIndex& sourceParent,
	    int sourceStart,
	    int sourceEnd,
	    const QModelIndex& destParent,
	    int destStart
	);

	void onModelAboutToBeReset();

private:
	void reloadElements();
	void insertModelElements(QAbstractItemModel* model, int first, int last);
	void insertComponent(qsizetype index, const QVariantMap& properties);
	void removeComponent(qsizetype index);

	QVariant mModel;
	QAbstractItemModel* itemModel = nullptr;
	QQmlComponent* mDelegate = nullptr;
};
