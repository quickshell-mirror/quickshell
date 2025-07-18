#include "server.hpp"
#include <functional>

#include <qcontainerfwd.h>
#include <qcoreapplication.h>
#include <qdbusconnection.h>
#include <qdbusmetatype.h>
#include <qdbusservicewatcher.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qqmlengine.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/logcat.hpp"
#include "../../core/model.hpp"
#include "dbus_notifications.h"
#include "dbusimage.hpp"
#include "notification.hpp"

namespace qs::service::notifications {

// NOLINTNEXTLINE(misc-use-internal-linkage)
QS_LOGGING_CATEGORY(logNotifications, "quickshell.service.notifications", QtWarningMsg);

NotificationServer::NotificationServer() {
	qDBusRegisterMetaType<DBusNotificationImage>();

	new DBusNotificationServer(this);

	qCInfo(logNotifications) << "Starting notification server";

	auto bus = QDBusConnection::sessionBus();

	if (!bus.isConnected()) {
		qCWarning(logNotifications) << "Could not connect to DBus. Notification service will not work.";
		return;
	}

	if (!bus.registerObject("/org/freedesktop/Notifications", this)) {
		qCWarning(logNotifications) << "Could not register Notification server object with DBus. "
		                               "Notification server will not work.";
		return;
	}

	QObject::connect(
	    &this->serviceWatcher,
	    &QDBusServiceWatcher::serviceUnregistered,
	    this,
	    &NotificationServer::onServiceUnregistered
	);

	this->serviceWatcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
	this->serviceWatcher.addWatchedService("org.freedesktop.Notifications");
	this->serviceWatcher.setConnection(bus);

	NotificationServer::tryRegister();
}

NotificationServer* NotificationServer::instance() {
	static auto* instance = new NotificationServer(); // NOLINT
	return instance;
}

void NotificationServer::switchGeneration(bool reEmit, const std::function<void()>& clearHook) {
	auto notifications = this->mNotifications.valueList();
	this->mNotifications.valueList().clear();
	this->idMap.clear();

	clearHook();

	if (reEmit) {
		for (auto* notification: notifications) {
			notification->setLastGeneration();
			notification->setTracked(false);
			emit this->notification(notification);

			if (!notification->isTracked()) {
				emit this->NotificationClosed(notification->id(), notification->closeReason());
				delete notification;
			} else {
				this->idMap.insert(notification->id(), notification);
				this->mNotifications.insertObject(notification);
			}
		}
	} else {
		for (auto* notification: notifications) {
			emit this->NotificationClosed(notification->id(), NotificationCloseReason::Expired);
			delete notification;
		}
	}
}

ObjectModel<Notification>* NotificationServer::trackedNotifications() {
	return &this->mNotifications;
}

void NotificationServer::deleteNotification(
    Notification* notification,
    NotificationCloseReason::Enum reason
) {
	if (!this->idMap.contains(notification->id())) return;

	emit notification->closed(reason);

	this->mNotifications.removeObject(notification);
	this->idMap.remove(notification->id());

	emit this->NotificationClosed(notification->id(), reason);
	notification->retainedDestroy();
}

void NotificationServer::tryRegister() {
	auto bus = QDBusConnection::sessionBus();
	auto success = bus.registerService("org.freedesktop.Notifications");

	if (success) {
		qCInfo(logNotifications) << "Registered notification server with dbus.";
	} else {
		qCWarning(logNotifications
		) << "Could not register notification server at org.freedesktop.Notifications, presumably "
		     "because one is already registered.";
		qCWarning(logNotifications
		) << "Registration will be attempted again if the active service is unregistered.";
	}
}
void NotificationServer::onServiceUnregistered(const QString& /*unused*/) {
	qCDebug(logNotifications) << "Active notification server unregistered, attempting registration";
	NotificationServer::tryRegister();
}

void NotificationServer::CloseNotification(uint id) {
	auto* notification = this->idMap.value(id);

	if (notification) {
		this->deleteNotification(notification, NotificationCloseReason::CloseRequested);
	}
}

QStringList NotificationServer::GetCapabilities() const {
	auto capabilities = QStringList();

	if (this->support.persistence) capabilities += "persistence";

	if (this->support.body) {
		capabilities += "body";
		if (this->support.bodyMarkup) capabilities += "body-markup";
		if (this->support.bodyHyperlinks) capabilities += "body-hyperlinks";
		if (this->support.bodyImages) capabilities += "body-images";
	}

	if (this->support.actions) {
		capabilities += "actions";
		if (this->support.actionIcons) capabilities += "action-icons";
	}

	if (this->support.image) capabilities += "icon-static";
	if (this->support.inlineReply) capabilities += "inline-reply";

	capabilities += this->support.extraHints;

	return capabilities;
}

QString
NotificationServer::GetServerInformation(QString& vendor, QString& version, QString& specVersion) {
	vendor = "quickshell";
	version = QCoreApplication::applicationVersion();
	specVersion = "1.2";
	return "quickshell";
}

uint NotificationServer::Notify(
    const QString& appName,
    uint replacesId,
    const QString& appIcon,
    const QString& summary,
    const QString& body,
    const QStringList& actions,
    const QVariantMap& hints,
    int expireTimeout
) {
	auto* notification = replacesId == 0 ? nullptr : this->idMap.value(replacesId);
	auto old = notification != nullptr;

	if (!notification) {
		notification = new Notification(this->nextId++, this);
		QQmlEngine::setObjectOwnership(notification, QQmlEngine::CppOwnership);
	}

	notification->updateProperties(appName, appIcon, summary, body, actions, hints, expireTimeout);

	if (!old) {
		emit this->notification(notification);

		if (!notification->isTracked()) {
			emit this->NotificationClosed(notification->id(), notification->closeReason());
			delete notification;
		} else {
			this->idMap.insert(notification->id(), notification);
			this->mNotifications.insertObject(notification);
		}
	}

	return notification->id();
}

} // namespace qs::service::notifications
