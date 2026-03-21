#pragma once

#include <functional>

#include <QtCore/qtmetamacros.h>
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
	Q_PROPERTY(QList<QObject*> values READ values NOTIFY valuesChanged);
	QML_NAMED_ELEMENT(ObjectModel);
	QML_UNCREATABLE("ObjectModels cannot be created directly.");

public:
	explicit UntypedObjectModel(QObject* parent): QAbstractListModel(parent) {}

	[[nodiscard]] QHash<int, QByteArray> roleNames() const override;

	[[nodiscard]] virtual QList<QObject*> values() = 0;

	Q_INVOKABLE virtual qsizetype indexOf(QObject* object) const = 0;

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

private:
	static qsizetype valuesCount(QQmlListProperty<QObject>* property);
	static QObject* valueAt(QQmlListProperty<QObject>* property, qsizetype index);
};

template <typename T>
class ObjectModel: public UntypedObjectModel {
public:
	explicit ObjectModel(QObject* parent): UntypedObjectModel(parent) {}

	[[nodiscard]] const QList<T*>& valueList() const { return this->mValuesList; }
	[[nodiscard]] QList<T*>& valueList() { return this->mValuesList; }

	void insertObject(T* object, qsizetype index = -1) {
		auto iindex = index == -1 ? this->mValuesList.length() : index;
		emit this->objectInsertedPre(object, iindex);

		auto intIndex = static_cast<qint32>(iindex);
		this->beginInsertRows(QModelIndex(), intIndex, intIndex);
		this->mValuesList.insert(iindex, object);
		this->endInsertRows();

		emit this->valuesChanged();
		emit this->objectInsertedPost(object, iindex);
	}

	void insertObjectSorted(T* object, const std::function<bool(T*, T*)>& compare) {
		auto& list = this->valueList();
		auto iter = list.begin();

		while (iter != list.end()) {
			if (!compare(object, *iter)) break;
			++iter;
		}

		auto idx = iter - list.begin();
		this->insertObject(object, idx);
	}

	bool removeObject(const T* object) {
		auto index = this->mValuesList.indexOf(object);
		if (index == -1) return false;

		this->removeAt(index);
		return true;
	}

	void removeAt(qsizetype index) {
		auto* object = this->mValuesList.at(index);
		emit this->objectRemovedPre(object, index);

		auto intIndex = static_cast<qint32>(index);
		this->beginRemoveRows(QModelIndex(), intIndex, intIndex);
		this->mValuesList.removeAt(index);
		this->endRemoveRows();

		emit this->valuesChanged();
		emit this->objectRemovedPost(object, index);
	}

	// Assumes only one instance of a specific value
	void diffUpdate(const QList<T*>& newValues) {
		for (qsizetype i = 0; i < this->mValuesList.length();) {
			if (newValues.contains(this->mValuesList.at(i))) i++;
			else this->removeAt(i);
		}

		qsizetype oi = 0;
		for (auto* object: newValues) {
			if (this->mValuesList.length() == oi || this->mValuesList.at(oi) != object) {
				this->insertObject(object, oi);
			}

			oi++;
		}
	}

	static ObjectModel<T>* emptyInstance() {
		return static_cast<ObjectModel<T>*>(UntypedObjectModel::emptyInstance());
	}

	[[nodiscard]] qint32 rowCount(const QModelIndex& parent) const override {
		if (parent != QModelIndex()) return 0;
		return static_cast<qint32>(this->mValuesList.length());
	}

	[[nodiscard]] QVariant data(const QModelIndex& index, qint32 role) const override {
		if (role != Qt::UserRole) return QVariant();
		// Values must be QObject derived, but we can't assert that here without breaking forward decls,
		// so no static_cast.
		return QVariant::fromValue(reinterpret_cast<QObject*>(this->mValuesList.at(index.row())));
	}

	qsizetype indexOf(QObject* object) const override {
		return this->mValuesList.indexOf(reinterpret_cast<T*>(object));
	}

	[[nodiscard]] QList<QObject*> values() override {
		return *reinterpret_cast<QList<QObject*>*>(&this->mValuesList);
	}

private:
	QList<T*> mValuesList;
};
