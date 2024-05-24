#include "model.hpp"

#include <qabstractitemmodel.h>
#include <qhash.h>
#include <qobject.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

qint32 UntypedObjectModel::rowCount(const QModelIndex& parent) const {
	if (parent != QModelIndex()) return 0;
	return static_cast<qint32>(this->valuesList.length());
}

QVariant UntypedObjectModel::data(const QModelIndex& index, qint32 role) const {
	if (role != 0) return QVariant();
	return QVariant::fromValue(this->valuesList.at(index.row()));
}

QHash<int, QByteArray> UntypedObjectModel::roleNames() const { return {{0, "modelData"}}; }

QQmlListProperty<QObject> UntypedObjectModel::values() {
	return QQmlListProperty<QObject>(
	    this,
	    nullptr,
	    &UntypedObjectModel::valuesCount,
	    &UntypedObjectModel::valueAt
	);
}

qsizetype UntypedObjectModel::valuesCount(QQmlListProperty<QObject>* property) {
	return static_cast<UntypedObjectModel*>(property->object)->valuesList.count(); // NOLINT
}

QObject* UntypedObjectModel::valueAt(QQmlListProperty<QObject>* property, qsizetype index) {
	return static_cast<UntypedObjectModel*>(property->object)->valuesList.at(index); // NOLINT
}

void UntypedObjectModel::insertObject(QObject* object, qsizetype index) {
	auto iindex = index == -1 ? this->valuesList.length() : index;
	emit this->objectInsertedPre(object, index);

	auto intIndex = static_cast<qint32>(iindex);
	this->beginInsertRows(QModelIndex(), intIndex, intIndex);
	this->valuesList.insert(iindex, object);
	this->endInsertRows();

	emit this->valuesChanged();
	emit this->objectInsertedPost(object, index);
}

void UntypedObjectModel::removeAt(qsizetype index) {
	auto* object = this->valuesList.at(index);
	emit this->objectRemovedPre(object, index);

	auto intIndex = static_cast<qint32>(index);
	this->beginRemoveRows(QModelIndex(), intIndex, intIndex);
	this->valuesList.removeAt(index);
	this->endRemoveRows();

	emit this->valuesChanged();
	emit this->objectRemovedPost(object, index);
}

bool UntypedObjectModel::removeObject(const QObject* object) {
	auto index = this->valuesList.indexOf(object);
	if (index == -1) return false;

	this->removeAt(index);
	return true;
}

qsizetype UntypedObjectModel::indexOf(QObject* object) { return this->valuesList.indexOf(object); }
