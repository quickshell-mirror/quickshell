#include "model.hpp"

#include <qabstractitemmodel.h>
#include <qhash.h>
#include <qnamespace.h>
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
	if (role != Qt::UserRole) return QVariant();
	return QVariant::fromValue(this->valuesList.at(index.row()));
}

QHash<int, QByteArray> UntypedObjectModel::roleNames() const {
	return {{Qt::UserRole, "modelData"}};
}

void UntypedObjectModel::insertObject(QObject* object, qsizetype index) {
	auto iindex = index == -1 ? this->valuesList.length() : index;
	emit this->objectInsertedPre(object, iindex);

	auto intIndex = static_cast<qint32>(iindex);
	this->beginInsertRows(QModelIndex(), intIndex, intIndex);
	this->valuesList.insert(iindex, object);
	this->endInsertRows();

	emit this->valuesChanged();
	emit this->objectInsertedPost(object, iindex);
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

void UntypedObjectModel::diffUpdate(const QVector<QObject*>& newValues) {
	for (qsizetype i = 0; i < this->valuesList.length();) {
		if (newValues.contains(this->valuesList.at(i))) i++;
		else this->removeAt(i);
	}

	qsizetype oi = 0;
	for (auto* object: newValues) {
		if (this->valuesList.length() == oi || this->valuesList.at(oi) != object) {
			this->insertObject(object, oi);
		}

		oi++;
	}
}

qsizetype UntypedObjectModel::indexOf(QObject* object) { return this->valuesList.indexOf(object); }

UntypedObjectModel* UntypedObjectModel::emptyInstance() {
	static auto* instance = new UntypedObjectModel(nullptr); // NOLINT
	return instance;
}
