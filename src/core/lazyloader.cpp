#include "lazyloader.hpp"
#include <utility>

#include <qlogging.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlcontext.h>
#include <qqmlengine.h>
#include <qqmlincubator.h>
#include <qtmetamacros.h>

#include "incubator.hpp"
#include "reload.hpp"

void LazyLoader::onReload(QObject* oldInstance) {
	auto* old = qobject_cast<LazyLoader*>(oldInstance);

	this->incubateIfReady(true);

	if (old != nullptr && old->mItem != nullptr && this->incubator != nullptr) {
		this->incubator->forceCompletion();
	}

	if (this->mItem != nullptr) {
		if (auto* reloadable = qobject_cast<Reloadable*>(this->mItem)) {
			reloadable->reload(old == nullptr ? nullptr : old->mItem);
		} else {
			Reloadable::reloadRecursive(this->mItem, old);
		}
	}
}

QObject* LazyLoader::item() {
	if (this->isLoading()) this->setActive(true);
	return this->mItem;
}

void LazyLoader::setItem(QObject* item) {
	if (item == this->mItem) return;

	if (this->mItem != nullptr) {
		this->mItem->deleteLater();
	}

	this->mItem = item;

	if (item != nullptr) {
		item->setParent(this);
	}

	this->targetActive = this->isActive();

	emit this->itemChanged();
	emit this->activeChanged();
}

bool LazyLoader::isLoading() const { return this->incubator != nullptr; }

void LazyLoader::setLoading(bool loading) {
	if (loading == this->targetLoading || this->isActive()) return;
	this->targetLoading = loading;

	if (loading) {
		this->incubateIfReady();
	} else if (this->mItem != nullptr) {
		this->mItem->deleteLater();
		this->mItem = nullptr;
	} else if (this->incubator != nullptr) {
		delete this->incubator;
		this->incubator = nullptr;
	}
}

bool LazyLoader::isActive() const { return this->mItem != nullptr; }

void LazyLoader::setActive(bool active) {
	if (active == this->targetActive) return;
	this->targetActive = active;

	if (active) {
		if (this->isLoading()) {
			this->incubator->forceCompletion();
		} else if (!this->isActive()) {
			this->incubateIfReady();
		}
	} else if (this->isActive()) {
		this->setItem(nullptr);
	}
}

void LazyLoader::setActiveAsync(bool active) {
	if (active == (this->targetActive || this->targetLoading)) return;
	if (active) this->setLoading(true);
	else this->setActive(false);
}

QQmlComponent* LazyLoader::component() const {
	return this->cleanupComponent ? nullptr : this->mComponent;
}

void LazyLoader::setComponent(QQmlComponent* component) {
	if (this->cleanupComponent) this->setSource(nullptr);
	if (component == this->mComponent) return;
	this->cleanupComponent = false;

	if (this->mComponent != nullptr) {
		QObject::disconnect(this->mComponent, nullptr, this, nullptr);
	}

	this->mComponent = component;

	if (component != nullptr) {
		QObject::connect(
		    this->mComponent,
		    &QObject::destroyed,
		    this,
		    &LazyLoader::onComponentDestroyed
		);
	}

	emit this->componentChanged();
}

void LazyLoader::onComponentDestroyed() {
	this->mComponent = nullptr;
	// todo: figure out what happens to the incubator
}

QString LazyLoader::source() const { return this->mSource; }

void LazyLoader::setSource(QString source) {
	if (!this->cleanupComponent) this->setComponent(nullptr);
	if (source == this->mSource) return;
	this->cleanupComponent = true;

	this->mSource = std::move(source);
	delete this->mComponent;

	if (!this->mSource.isEmpty()) {
		auto* context = QQmlEngine::contextForObject(this);
		this->mComponent = new QQmlComponent(
		    context == nullptr ? nullptr : context->engine(),
		    context == nullptr ? this->mSource : context->resolvedUrl(this->mSource)
		);

		if (this->mComponent->isError()) {
			qWarning() << this->mComponent->errorString().toStdString().c_str();
			delete this->mComponent;
			this->mComponent = nullptr;
		}
	} else {
		this->mComponent = nullptr;
	}

	emit this->sourceChanged();
}

void LazyLoader::incubateIfReady(bool overrideReloadCheck) {
	if (!(this->reloadComplete || overrideReloadCheck) || !(this->targetLoading || this->targetActive)
	    || this->mComponent == nullptr || this->incubator != nullptr)
	{
		return;
	}

	this->incubator = new QsQmlIncubator(
	    this->targetActive ? QQmlIncubator::Synchronous : QQmlIncubator::Asynchronous,
	    this
	);

	// clang-format off
	QObject::connect(this->incubator, &QsQmlIncubator::completed, this, &LazyLoader::onIncubationCompleted);
	QObject::connect(this->incubator, &QsQmlIncubator::failed, this, &LazyLoader::onIncubationFailed);
	// clang-format on

	emit this->loadingChanged();

	this->mComponent->create(*this->incubator, QQmlEngine::contextForObject(this->mComponent));
}

void LazyLoader::onIncubationCompleted() {
	this->setItem(this->incubator->object());
	// The incubator is not necessarily inert at the time of this callback,
	// so deleteLater is required.
	this->incubator->deleteLater();
	this->incubator = nullptr;
	this->targetLoading = false;
	emit this->loadingChanged();
}

void LazyLoader::onIncubationFailed() {
	qWarning() << "Failed to create LazyLoader component";

	for (auto& error: this->incubator->errors()) {
		qWarning() << error;
	}

	delete this->incubator;
	this->targetLoading = false;
	emit this->loadingChanged();
}
