#include "bus.hpp" // NOLINT
#include <functional>

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qdbusmessage.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtypes.h>
#include <qvariant.h>

#include "../core/logcat.hpp"

namespace qs::dbus {

namespace {
QS_LOGGING_CATEGORY(logDbus, "quickshell.dbus", QtWarningMsg);
}

void tryLaunchService(
    QObject* parent,
    QDBusConnection& connection,
    const QString& serviceName,
    const std::function<void(bool)>& callback
) {
	qCDebug(logDbus) << "Attempting to launch service" << serviceName;

	auto message = QDBusMessage::createMethodCall(
	    "org.freedesktop.DBus",
	    "/org/freedesktop/DBus",
	    "org.freedesktop.DBus",
	    "StartServiceByName"
	);

	message << serviceName << 0u;
	auto pendingCall = connection.asyncCall(message);

	auto* call = new QDBusPendingCallWatcher(pendingCall, parent);

	auto responseCallback = [callback, serviceName](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<quint32> reply = *call;

		if (reply.isError()) {
			qCWarning(logDbus).noquote().nospace()
			    << "Could not launch service " << serviceName << ": " << reply.error();
			callback(false);
		} else {
			qCDebug(logDbus) << "Service launch successful for" << serviceName;
			callback(true);
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, parent, responseCallback);
}

} // namespace qs::dbus
