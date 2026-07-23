#pragma once

#include <qobject.h>
#include <qstring.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwindow.h>

class QSWaylandSessionLock;
class QSWaylandSessionLockSurface;
class QSWaylandSessionLockIntegration;

class SessionLockManager: public QObject {
	Q_OBJECT

public:
	explicit SessionLockManager(bool logindLockedHint, QObject* parent = nullptr);

	[[nodiscard]] static bool lockAvailable();

	bool lock();
	bool unlock();

	[[nodiscard]] bool isLocked() const;

	static bool sessionLocked();
	static bool isSecure();

signals:
	// This signal is sent once the compositor considers the session to be fully locked.
	// This corresponds to the ext_session_lock_v1::locked event.
	void locked();

	// This signal is sent once the compositor considers the session to be unlocked.
	// This corresponds to the ext_session_lock_v1::finished event.
	//
	// The session lock will end in one of three cases.
	// 1. unlock() is called.
	// 2. The SessionLockManager is destroyed.
	// 3. The compositor forcibly unlocks the session.
	//
	// After receiving this event the caller should destroy all of its lock surfaces.
	void unlocked();

private:
	void updateLogindLockedHint(bool locked);
	void updateLogindLockedHint(const QString& sessionPath, bool locked, quint64 generation);

	QSWaylandSessionLock* mLock = nullptr;
	QString mLogindSessionPath;
	quint64 mLogindLockedHintGeneration = 0;
	bool mLogindLockedHint = false;

	friend class LockWindowExtension;
};

class LockWindowExtension: public QObject {
	Q_OBJECT

public:
	explicit LockWindowExtension(QObject* parent = nullptr): QObject(parent) {}
	~LockWindowExtension() override;
	Q_DISABLE_COPY_MOVE(LockWindowExtension)

	[[nodiscard]] bool isAttached() const;

	bool attach(QWindow* window, SessionLockManager* manager);

	void setVisible();

	static LockWindowExtension* get(QWindow* window);

signals:
	void locked();
	void unlocked();

private:
	QSWaylandSessionLockSurface* surface = nullptr;
	QSWaylandSessionLock* lock = nullptr;
	bool immediatelyVisible = false;

	friend class QSWaylandSessionLockSurface;
	friend class QSWaylandSessionLockIntegration;
};
