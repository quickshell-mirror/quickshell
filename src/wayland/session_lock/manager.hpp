#pragma once

#include <qtclasshelpermacros.h>
#include <qwayland-ext-session-lock-v1.h>
#include <qwaylandclientextension.h>

#include "lock.hpp"

class QSWaylandSessionLockManager
    : public QWaylandClientExtensionTemplate<QSWaylandSessionLockManager>
    , public QtWayland::ext_session_lock_manager_v1 {
public:
	QSWaylandSessionLockManager();
	~QSWaylandSessionLockManager() override;
	Q_DISABLE_COPY_MOVE(QSWaylandSessionLockManager);

	// Create a new session lock if there is no currently active lock, otherwise null.
	QSWaylandSessionLock* acquireLock();
	[[nodiscard]] bool isLocked() const;
	[[nodiscard]] bool isSecure() const;

	static bool sessionLocked();

private:
	QSWaylandSessionLock* active = nullptr;

	friend class QSWaylandSessionLock;
};
