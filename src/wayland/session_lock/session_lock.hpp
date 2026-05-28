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
	void locked();
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
