#pragma once

#include <bit>
#include <qabstractitemmodel.h>
#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "doc.hpp"

///! View into a list of objets
/// Typed view into a list of objects.
///
/// An ObjectModel works as a QML [Data Model], allowing efficient interaction with
/// components that act on models. It has a single role named `modelData`, to match the
/// behavior of lists.
/// The same information contained in the list model is available as a normal list
/// via the `values` property.
///
/// #### Differences from a list
/// Unlike with a list, the following property binding will never be updated when `model[3]` changes.
/// ```qml
/// // will not update reactively
/// property var foo: model[3]
/// ```
///
/// You can work around this limitation using the @@values property of the model to view it as a list.
/// ```qml
/// // will update reactively
/// property var foo: model.values[3]
/// ```
///
/// [Data Model]: https://doc.qt.io/qt-6/qtquick-modelviewsdata-modelview.html#qml-data-models
class UntypedObjectModel: public QAbstractListModel {
	QSDOC_CNAME(ObjectModel);
	Q_OBJECT;
	/// The content of the object model, as a QML list.
	/// The values of this property will always be of the type of the model.
	Q_PROPERTY(QQmlListProperty<QObject> values READ values NOTIFY valuesChanged);
	QML_NAMED_ELEMENT(ObjectModel);
	QML_UNCREATABLE("ObjectModels cannot be created directly.");

public:
	explicit UntypedObjectModel(QObject* parent): QAbstractListModel(parent) {}

	[[nodiscard]] qint32 rowCount(const QModelIndex& parent) const override;
	[[nodiscard]] QVariant data(const QModelIndex& index, qint32 role) const override;
	[[nodiscard]] QHash<int, QByteArray> roleNames() const override;

	[[nodiscard]] QQmlListProperty<QObject> values();
	void removeAt(qsizetype index);

	Q_INVOKABLE qsizetype indexOf(QObject* object);

	static UntypedObjectModel* emptyInstance();

signals:
	void valuesChanged();
	/// Sent immediately before an object is inserted into the list.
	void objectInsertedPre(QObject* object, qsizetype index);
	/// Sent immediately after an object is inserted into the list.
	void objectInsertedPost(QObject* object, qsizetype index);
	/// Sent immediately before an object is removed from the list.
	void objectRemovedPre(QObject* object, qsizetype index);
	/// Sent immediately after an object is removed from the list.
	void objectRemovedPost(QObject* object, qsizetype index);

protected:
	void insertObject(QObject* object, qsizetype index = -1);
	bool removeObject(const QObject* object);

	QVector<QObject*> valuesList;

private:
	static qsizetype valuesCount(QQmlListProperty<QObject>* property);
	static QObject* valueAt(QQmlListProperty<QObject>* property, qsizetype index);
};

template <typename T>
class ObjectModel: public UntypedObjectModel {
public:
	explicit ObjectModel(QObject* parent): UntypedObjectModel(parent) {}

	[[nodiscard]] QVector<T*>& valueList() { return *std::bit_cast<QVector<T*>*>(&this->valuesList); }

	[[nodiscard]] const QVector<T*>& valueList() const {
		return *std::bit_cast<const QVector<T*>*>(&this->valuesList);
	}

	void insertObject(T* object, qsizetype index = -1) {
		this->UntypedObjectModel::insertObject(object, index);
	}

	void removeObject(const T* object) { this->UntypedObjectModel::removeObject(object); }

	static ObjectModel<T>* emptyInstance() {
		return static_cast<ObjectModel<T>*>(UntypedObjectModel::emptyInstance());
	}
};
