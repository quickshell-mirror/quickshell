#include "retainable.hpp"

#include <qlogging.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qvariant.h>

RetainableHook* RetainableHook::getHook(QObject* object, bool create) {
	auto v = object->property("__qs_retainable");

	if (v.canConvert<RetainableHook*>()) {
		return v.value<RetainableHook*>();
	} else if (create) {
		auto* retainable = dynamic_cast<Retainable*>(object);
		if (!retainable) return nullptr;

		auto* hook = new RetainableHook(object);
		hook->retainableFacet = retainable;
		retainable->hook = hook;

		object->setProperty("__qs_retainable", QVariant::fromValue(hook));

		return hook;
	} else return nullptr;
}

RetainableHook* RetainableHook::qmlAttachedProperties(QObject* object) {
	return RetainableHook::getHook(object, true);
}

void RetainableHook::ref() { this->refcount++; }

void RetainableHook::unref() {
	this->refcount--;
	if (this->refcount == 0) this->unlocked();
}

void RetainableHook::lock() {
	this->explicitRefcount++;
	this->ref();
}

void RetainableHook::unlock() {
	if (this->explicitRefcount < 1) {
		qWarning() << "Retainable object" << this->parent()
		           << "unlocked more times than it was locked!";
	} else {
		this->explicitRefcount--;
		this->unref();
	}
}

void RetainableHook::forceUnlock() { this->unlocked(); }

bool RetainableHook::isRetained() const { return !this->inactive; }

void RetainableHook::unlocked() {
	if (this->inactive) return;

	emit this->aboutToDestroy();
	this->retainableFacet->retainFinished();
}

void Retainable::retainedDestroy() {
	this->retaining = true;

	auto* hook = RetainableHook::getHook(dynamic_cast<QObject*>(this), false);

	if (hook) {
		// let all signal handlers run before acting on changes
		emit hook->dropped();
		hook->inactive = false;

		if (hook->refcount == 0) hook->unlocked();
		else emit hook->retainedChanged();
	} else {
		this->retainFinished();
	}
}

bool Retainable::isRetained() const { return this->retaining; }

void Retainable::retainFinished() {
	// a normal delete tends to cause deref errors in a listview.
	dynamic_cast<QObject*>(this)->deleteLater();
}

RetainableLock::~RetainableLock() {
	if (this->mEnabled && this->mObject) {
		this->hook->unref();
	}
}

QObject* RetainableLock::object() const { return this->mObject; }

void RetainableLock::setObject(QObject* object) {
	if (object == this->mObject) return;

	if (this->mObject) {
		QObject::disconnect(this->mObject, nullptr, this, nullptr);
		if (this->hook->isRetained()) emit this->retainedChanged();
		this->hook->unref();
	}

	this->mObject = nullptr;
	this->hook = nullptr;

	if (object) {
		if (auto* hook = RetainableHook::getHook(object, true)) {
			this->mObject = object;
			this->hook = hook;

			QObject::connect(object, &QObject::destroyed, this, &RetainableLock::onObjectDestroyed);
			QObject::connect(hook, &RetainableHook::dropped, this, &RetainableLock::dropped);
			QObject::connect(
			    hook,
			    &RetainableHook::aboutToDestroy,
			    this,
			    &RetainableLock::aboutToDestroy
			);
			QObject::connect(
			    hook,
			    &RetainableHook::retainedChanged,
			    this,
			    &RetainableLock::retainedChanged
			);
			if (hook->isRetained()) emit this->retainedChanged();

			hook->ref();
		} else {
			qCritical() << "Tried to set non retainable object" << object << "as the target of" << this;
		}
	}

	emit this->objectChanged();
}

void RetainableLock::onObjectDestroyed() {
	this->mObject = nullptr;
	this->hook = nullptr;

	emit this->objectChanged();
}

bool RetainableLock::locked() const { return this->mEnabled; }

void RetainableLock::setLocked(bool locked) {
	if (locked == this->mEnabled) return;

	this->mEnabled = locked;

	if (this->mObject) {
		if (locked) this->hook->ref();
		else {
			if (this->hook->isRetained()) emit this->retainedChanged();
			this->hook->unref();
		}
	}

	emit this->lockedChanged();
}

bool RetainableLock::isRetained() const { return this->mObject && this->hook->isRetained(); }
