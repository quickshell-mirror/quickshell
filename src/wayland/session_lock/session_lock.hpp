#pragma once

#include <qobject.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qwindow.h>

class QSWaylandSessionLock;
class QSWaylandSessionLockSurface;
class QSWaylandSessionLockIntegration;

class SessionLockManager: public QObject {
	Q_OBJECT;

public:
	explicit SessionLockManager(QObject* parent = nullptr): QObject(parent) {}

	[[nodiscard]] static bool lockAvailable();

	// Returns true if a lock was acquired.
	// If true is returned the caller must watch the global screen list and create/destroy
	// windows with an attached LockWindowExtension to match it.
	bool lock();

	// Returns true if the session was locked and is now unlocked.
	bool unlock();

	[[nodiscard]] bool isLocked() const;

	static bool sessionLocked();
	static bool isSecure();

signals:
	// This signal is sent once the compositor considers the session to be fully locked.
	// This corrosponds to the ext_session_lock_v1::locked event.
	void locked();

	// This signal is sent once the compositor considers the session to be unlocked.
	// This corrosponds to the ext_session_lock_v1::finished event.
	//
	// The session lock will end in one of three cases.
	// 1. unlock() is called.
	// 2. The SessionLockManager is destroyed.
	// 3. The compositor forcibly unlocks the session.
	//
	// After receiving this event the caller should destroy all of its lock surfaces.
	void unlocked();

private slots:
	//void onUnlocked();

private:
	QSWaylandSessionLock* mLock = nullptr;

	friend class LockWindowExtension;
};

class LockWindowExtension: public QObject {
	Q_OBJECT;

public:
	explicit LockWindowExtension(QObject* parent = nullptr): QObject(parent) {}
	~LockWindowExtension() override;
	Q_DISABLE_COPY_MOVE(LockWindowExtension);

	[[nodiscard]] bool isAttached() const;

	// Attach this lock extension to the given window.
	// The extension is reparented to the window and replaces any existing lock extension.
	// Returns false if the window cannot be used.
	bool attach(QWindow* window, SessionLockManager* manager);

	// This must be called in place of QWindow::setVisible. Calling QWindow::setVisible will result in a crash.
	// To make a window invisible, destroy it as it cannot be recovered.
	void setVisible();

	static LockWindowExtension* get(QWindow* window);

signals:
	// This signal is sent once the compositor considers the session to be fully locked.
	// See SessionLockManager::locked for details.
	void locked();

	// After receiving this signal the window is no longer in use by the compositor
	// and should be destroyed. See SessionLockManager::unlocked for details.
	void unlocked();

private:
	QSWaylandSessionLockSurface* surface = nullptr;
	QSWaylandSessionLock* lock = nullptr;
	bool immediatelyVisible = false;

	friend class QSWaylandSessionLockSurface;
	friend class QSWaylandSessionLockIntegration;
};
