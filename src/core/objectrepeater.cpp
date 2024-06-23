#include "objectrepeater.hpp"
#include <utility>

#include <qabstractitemmodel.h>
#include <qcontainerfwd.h>
#include <qhash.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlcontext.h>
#include <qqmlengine.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

QVariant ObjectRepeater::model() const { return this->mModel; }

void ObjectRepeater::setModel(QVariant model) {
	if (model == this->mModel) return;

	if (this->itemModel != nullptr) {
		QObject::disconnect(this->itemModel, nullptr, this, nullptr);
	}

	this->mModel = std::move(model);
	emit this->modelChanged();
	this->reloadElements();
}

void ObjectRepeater::onModelDestroyed() {
	this->mModel.clear();
	this->itemModel = nullptr;
	emit this->modelChanged();
	this->reloadElements();
}

QQmlComponent* ObjectRepeater::delegate() const { return this->mDelegate; }

void ObjectRepeater::setDelegate(QQmlComponent* delegate) {
	if (delegate == this->mDelegate) return;

	if (this->mDelegate != nullptr) {
		QObject::disconnect(this->mDelegate, nullptr, this, nullptr);
	}

	this->mDelegate = delegate;

	if (delegate != nullptr) {
		QObject::connect(
		    this->mDelegate,
		    &QObject::destroyed,
		    this,
		    &ObjectRepeater::onDelegateDestroyed
		);
	}

	emit this->delegateChanged();
	this->reloadElements();
}

void ObjectRepeater::onDelegateDestroyed() {
	this->mDelegate = nullptr;
	emit this->delegateChanged();
	this->reloadElements();
}

void ObjectRepeater::reloadElements() {
	for (auto i = this->valuesList.length() - 1; i >= 0; i--) {
		this->removeComponent(i);
	}

	if (this->mDelegate == nullptr || !this->mModel.isValid()) return;

	if (this->mModel.canConvert<QAbstractItemModel*>()) {
		auto* model = this->mModel.value<QAbstractItemModel*>();
		this->itemModel = model;

		this->insertModelElements(model, 0, model->rowCount() - 1); // -1 is fine

		// clang-format off
		QObject::connect(model, &QObject::destroyed, this, &ObjectRepeater::onModelDestroyed);
		QObject::connect(model, &QAbstractItemModel::rowsInserted, this, &ObjectRepeater::onModelRowsInserted);
		QObject::connect(model, &QAbstractItemModel::rowsRemoved, this, &ObjectRepeater::onModelRowsRemoved);
		QObject::connect(model, &QAbstractItemModel::rowsMoved, this, &ObjectRepeater::onModelRowsMoved);
		QObject::connect(model, &QAbstractItemModel::modelAboutToBeReset, this, &ObjectRepeater::onModelAboutToBeReset);
		// clang-format on
	} else if (this->mModel.canConvert<QQmlListReference>()) {
		auto values = this->mModel.value<QQmlListReference>();
		auto len = values.count();

		for (auto i = 0; i != len; i++) {
			this->insertComponent(i, {{"modelData", QVariant::fromValue(values.at(i))}});
		}
	} else if (this->mModel.canConvert<QVector<QVariant>>()) {
		auto values = this->mModel.value<QVector<QVariant>>();

		for (auto& value: values) {
			this->insertComponent(this->valuesList.length(), {{"modelData", value}});
		}
	} else {
		qCritical() << this
		            << "Cannot create components as the model is not compatible:" << this->mModel;
	}
}

void ObjectRepeater::insertModelElements(QAbstractItemModel* model, int first, int last) {
	auto roles = model->roleNames();
	auto roleDataVec = QVector<QModelRoleData>();
	for (auto id: roles.keys()) {
		roleDataVec.push_back(QModelRoleData(id));
	}

	auto values = QModelRoleDataSpan(roleDataVec);
	auto props = QVariantMap();

	for (auto i = first; i != last + 1; i++) {
		auto index = model->index(i, 0);
		model->multiData(index, values);

		for (auto [id, name]: roles.asKeyValueRange()) {
			props.insert(name, *values.dataForRole(id));
		}

		this->insertComponent(i, props);

		props.clear();
	}
}

void ObjectRepeater::onModelRowsInserted(const QModelIndex& parent, int first, int last) {
	if (parent != QModelIndex()) return;

	this->insertModelElements(this->itemModel, first, last);
}

void ObjectRepeater::onModelRowsRemoved(const QModelIndex& parent, int first, int last) {
	if (parent != QModelIndex()) return;

	for (auto i = last; i != first - 1; i--) {
		this->removeComponent(i);
	}
}

void ObjectRepeater::onModelRowsMoved(
    const QModelIndex& sourceParent,
    int sourceStart,
    int sourceEnd,
    const QModelIndex& destParent,
    int destStart
) {
	auto hasSource = sourceParent != QModelIndex();
	auto hasDest = destParent != QModelIndex();

	if (!hasSource && !hasDest) return;

	if (hasSource) {
		this->onModelRowsRemoved(sourceParent, sourceStart, sourceEnd);
	}

	if (hasDest) {
		this->onModelRowsInserted(destParent, destStart, destStart + (sourceEnd - sourceStart));
	}
}

void ObjectRepeater::onModelAboutToBeReset() {
	auto last = static_cast<int>(this->valuesList.length() - 1);
	this->onModelRowsRemoved(QModelIndex(), 0, last); // -1 is fine
}

void ObjectRepeater::insertComponent(qsizetype index, const QVariantMap& properties) {
	auto* context = QQmlEngine::contextForObject(this);
	auto* instance = this->mDelegate->createWithInitialProperties(properties, context);

	if (instance == nullptr) {
		qWarning().noquote() << this->mDelegate->errorString();
		qWarning() << this << "failed to create object for model data" << properties;
	} else {
		QQmlEngine::setObjectOwnership(instance, QQmlEngine::CppOwnership);
		instance->setParent(this);
	}

	this->insertObject(instance, index);
}

void ObjectRepeater::removeComponent(qsizetype index) {
	auto* instance = this->valuesList.at(index);
	this->removeAt(index);
	delete instance;
}
