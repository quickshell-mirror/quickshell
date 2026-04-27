#include "session_lock_dbus.hpp"

#include <qdbusabstractadaptor.h>
#include <qdbusconnection.h>
#include <qdbuserror.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include "../core/logcat.hpp"

namespace {
QS_LOGGING_CATEGORY(logDbusSessionLock, "quickshell.dbus.sessionlock", QtWarningMsg);
}

namespace qs::dbus {

SessionLockAdaptor::SessionLockAdaptor(QObject* parent): QDBusAbstractAdaptor(parent) {
	// Register on session bus
	auto connection = QDBusConnection::sessionBus();

	const bool serviceOk = connection.registerService("org.quickshell.SessionLock");
	if (!serviceOk) {
		qCWarning(logDbusSessionLock) << "Failed to register DBus service org.quickshell.SessionLock:"
		                              << connection.lastError().message();
		return;
	}

	qCInfo(logDbusSessionLock) << "Registered DBus service org.quickshell.SessionLock";

	if (!connection
	         .registerObject("/org/quickshell/SessionLock", parent, QDBusConnection::ExportAdaptors))
	{
		qCWarning(logDbusSessionLock) << "Failed to register DBus object /org/quickshell/SessionLock:"
		                              << connection.lastError().message();
	} else {
		qCInfo(logDbusSessionLock) << "Registered DBus object /org/quickshell/SessionLock";
	}
}

void SessionLockAdaptor::setLocked(bool locked) {
	if (this->mLocked == locked) return;

	this->mLocked = locked;
	qCDebug(logDbusSessionLock) << "Lock state changed to:" << locked;

	emit this->LockedChanged(locked);
	emit this->lockedChanged(locked);
}

void SessionLockAdaptor::setSecure(bool secure) {
	if (this->mSecure == secure) return;

	this->mSecure = secure;
	qCDebug(logDbusSessionLock) << "Secure state changed to:" << secure;

	emit this->SecureChanged(secure);
	emit this->secureChanged(secure);
}

bool SessionLockAdaptor::GetLocked() const {
	qCDebug(logDbusSessionLock) << "GetLocked called, returning:" << this->mLocked;
	return this->mLocked;
}

bool SessionLockAdaptor::GetSecure() const {
	qCDebug(logDbusSessionLock) << "GetSecure called, returning:" << this->mSecure;
	return this->mSecure;
}

} // namespace qs::dbus
