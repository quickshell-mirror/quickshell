#pragma once
#include <qobject.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qwindow.h>

class QSWaylandSessionLock;
class QSWaylandSessionLockSurface;
class QSWaylandSessionLockIntegration;

namespace qs::dbus {
class SessionLockAdaptor;
}

class SessionLockManager: public QObject {
	Q_OBJECT

public:
	explicit SessionLockManager(bool exposeDbus, QObject* parent = nullptr);

	[[nodiscard]] static bool lockAvailable();

	bool lock();
	bool unlock();

	[[nodiscard]] bool isLocked() const;

	static bool sessionLocked();
	static bool isSecure();

signals:
	void locked();
	void unlocked();

private:
	QSWaylandSessionLock* mLock = nullptr;
	qs::dbus::SessionLockAdaptor* mDbusAdaptor = nullptr;

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
