#include "manager.hpp"

#include <qdebug.h>
#include <qlogging.h>
#include <qwaylandclientextension.h>

#include "lock.hpp"

QSWaylandSessionLockManager::QSWaylandSessionLockManager()
    : QWaylandClientExtensionTemplate<QSWaylandSessionLockManager>(1) {
	this->initialize();
}

QSWaylandSessionLockManager::~QSWaylandSessionLockManager() { this->destroy(); }

QSWaylandSessionLock* QSWaylandSessionLockManager::acquireLock() {
	if (this->isLocked()) return nullptr;
	this->active = new QSWaylandSessionLock(this, this->lock());
	return this->active;
}

bool QSWaylandSessionLockManager::isLocked() const { return this->active != nullptr; }
