#include "lock.hpp"

#include <qtmetamacros.h>
#include <wayland-ext-session-lock-v1-client-protocol.h>

#include "manager.hpp"

QSWaylandSessionLock::QSWaylandSessionLock(
    QSWaylandSessionLockManager* manager,
    ::ext_session_lock_v1* lock
)
    : manager(manager) {
	this->init(lock); // if isInitialized is false that means we already unlocked.
}

QSWaylandSessionLock::~QSWaylandSessionLock() {
	if (this->isInitialized()) {
		// This will intentionally lock the session if the lock is destroyed without calling unlock.
		this->destroy();
	}
}

void QSWaylandSessionLock::unlock() {
	if (this->isInitialized()) {
		if (this->finished) this->destroy();
		else this->unlock_and_destroy();

		this->secure = false;
		this->manager->active = nullptr;

		emit this->unlocked();
	}
}

bool QSWaylandSessionLock::active() const { return this->isInitialized(); }

bool QSWaylandSessionLock::hasCompositorLock() const { return this->secure; }

void QSWaylandSessionLock::ext_session_lock_v1_locked() {
	this->secure = true;
	emit this->compositorLocked();
}

void QSWaylandSessionLock::ext_session_lock_v1_finished() {
	this->secure = false;
	this->finished = true;
	this->unlock();
}
