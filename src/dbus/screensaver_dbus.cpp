#include "screensaver_dbus.hpp"

#include <qdbusabstractadaptor.h>
#include <qdbusconnection.h>
#include <qdbuserror.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include "../core/logcat.hpp"

namespace {

QS_LOGGING_CATEGORY(logDbusScreenSaver, "quickshell.dbus.screensaver", QtWarningMsg);

}

namespace qs::dbus {

ScreenSaverAdaptor::ScreenSaverAdaptor(QObject* parent): QDBusAbstractAdaptor(parent) {
	// Register on session bus
	auto connection = QDBusConnection::sessionBus();

	if (!connection.registerService("org.freedesktop.ScreenSaver")) {
		qCWarning(logDbusScreenSaver) << "Failed to register DBus service org.freedesktop.ScreenSaver:"
		                              << connection.lastError().message();
	} else {
		qCInfo(logDbusScreenSaver) << "Registered DBus service org.freedesktop.ScreenSaver";
	}

	if (!connection
	         .registerObject("/org/freedesktop/ScreenSaver", parent, QDBusConnection::ExportAdaptors))
	{
		qCWarning(logDbusScreenSaver) << "Failed to register DBus object /org/freedesktop/ScreenSaver:"
		                              << connection.lastError().message();
	} else {
		qCInfo(logDbusScreenSaver) << "Registered DBus object /org/freedesktop/ScreenSaver";
	}
}

void ScreenSaverAdaptor::setActive(bool active) {
	if (this->mActive != active) {
		this->mActive = active;
		qCDebug(logDbusScreenSaver) << "Lock state changed to:" << active;
		emit this->ActiveChanged(active);
	}
}

void ScreenSaverAdaptor::setSecure(bool secure) {
	if (this->mSecure != secure) {
		this->mSecure = secure;
		qCDebug(logDbusScreenSaver) << "Secure state changed to:" << secure;
	}
}

bool ScreenSaverAdaptor::GetActive() const {
	qCDebug(logDbusScreenSaver) << "GetActive called, returning:" << this->mActive;
	return this->mActive;
}

bool ScreenSaverAdaptor::GetSecure() const {
	qCDebug(logDbusScreenSaver) << "GetSecure called, returning:" << this->mSecure;
	return this->mSecure;
}

} // namespace qs::dbus
