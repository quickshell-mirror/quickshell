#pragma once

#include <qobject.h>
#include <qtclasshelpermacros.h>
#include <qwayland-ext-session-lock-v1.h>

class QSWaylandSessionLockManager;

class QSWaylandSessionLock
    : public QObject
    , public QtWayland::ext_session_lock_v1 {
	Q_OBJECT;

public:
	QSWaylandSessionLock(QSWaylandSessionLockManager* manager, ::ext_session_lock_v1* lock);
	~QSWaylandSessionLock() override;
	Q_DISABLE_COPY_MOVE(QSWaylandSessionLock);

	void unlock();

	// Returns true if the lock has not finished.
	[[nodiscard]] bool active() const;
	// Returns true if the compositor considers the session to be locked.
	[[nodiscard]] bool hasCompositorLock() const;

signals:
	void compositorLocked();
	void unlocked();

private:
	void ext_session_lock_v1_locked() override;
	void ext_session_lock_v1_finished() override;

	QSWaylandSessionLockManager* manager; // static and not dealloc'd

	// true when the compositor determines the session is locked
	bool secure = false;
	bool finished = false;
};
