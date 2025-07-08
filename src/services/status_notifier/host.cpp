#include "host.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qdbuserror.h>
#include <qdbusservicewatcher.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <unistd.h>

#include "../../core/common.hpp"
#include "../../core/logcat.hpp"
#include "../../dbus/properties.hpp"
#include "dbus_watcher_interface.h"
#include "item.hpp"
#include "watcher.hpp"

QS_LOGGING_CATEGORY(logStatusNotifierHost, "quickshell.service.sni.host", QtWarningMsg);

namespace qs::service::sni {

StatusNotifierHost::StatusNotifierHost(QObject* parent): QObject(parent) {
	StatusNotifierWatcher::instance(); // ensure at least one watcher exists

	auto bus = QDBusConnection::sessionBus();

	if (!bus.isConnected()) {
		qCWarning(logStatusNotifierHost)
		    << "Could not connect to DBus. StatusNotifier service will not work.";
		return;
	}

	this->hostId = QString("org.kde.StatusNotifierHost-%1-%2")
	                   .arg(QString::number(getpid()))
	                   .arg(QString::number(qs::Common::LAUNCH_TIME.toMSecsSinceEpoch()));

	auto success = bus.registerService(this->hostId);

	if (!success) {
		qCWarning(logStatusNotifierHost) << "Could not register StatusNotifierHost object with DBus. "
		                                    "StatusNotifer service will not work.";
		return;
	}

	QObject::connect(
	    &this->serviceWatcher,
	    &QDBusServiceWatcher::serviceRegistered,
	    this,
	    &StatusNotifierHost::onWatcherRegistered
	);

	QObject::connect(
	    &this->serviceWatcher,
	    &QDBusServiceWatcher::serviceUnregistered,
	    this,
	    &StatusNotifierHost::onWatcherUnregistered
	);

	this->serviceWatcher.addWatchedService("org.kde.StatusNotifierWatcher");
	this->serviceWatcher.setConnection(bus);

	this->watcher = new DBusStatusNotifierWatcher(
	    "org.kde.StatusNotifierWatcher",
	    "/StatusNotifierWatcher",
	    bus,
	    this
	);

	QObject::connect(
	    this->watcher,
	    &DBusStatusNotifierWatcher::StatusNotifierItemRegistered,
	    this,
	    &StatusNotifierHost::onItemRegistered
	);

	QObject::connect(
	    this->watcher,
	    &DBusStatusNotifierWatcher::StatusNotifierItemUnregistered,
	    this,
	    &StatusNotifierHost::onItemUnregistered
	);

	if (!this->watcher->isValid()) {
		qCWarning(logStatusNotifierHost)
		    << "Could not find active StatusNotifierWatcher. StatusNotifier service will not work "
		       "until one is present.";
		return;
	}

	this->connectToWatcher();
}

void StatusNotifierHost::connectToWatcher() {
	qCDebug(logStatusNotifierHost) << "Registering host with active StatusNotifierWatcher";
	this->watcher->RegisterStatusNotifierHost(this->hostId);

	qs::dbus::asyncReadProperty<QStringList>(
	    *this->watcher,
	    "RegisteredStatusNotifierItems",
	    [this](QStringList value, QDBusError error) { // NOLINT
		    if (error.isValid()) {
			    qCWarning(logStatusNotifierHost).noquote()
			        << "Error reading \"RegisteredStatusNotifierItems\" property of watcher"
			        << this->watcher->service();

			    qCWarning(logStatusNotifierHost) << error;
		    } else {
			    qCDebug(logStatusNotifierHost)
			        << "Registering preexisting status notifier items from watcher:" << value;

			    for (auto& item: value) {
				    this->onItemRegistered(item);
			    }
		    }
	    }
	);
}

QList<StatusNotifierItem*> StatusNotifierHost::items() const {
	auto items = this->mItems.values();
	items.removeIf([](StatusNotifierItem* item) { return !item->isReady(); });
	return items;
}

StatusNotifierItem* StatusNotifierHost::itemByService(const QString& service) const {
	return this->mItems.value(service);
}

void StatusNotifierHost::onWatcherRegistered() { this->connectToWatcher(); }

void StatusNotifierHost::onWatcherUnregistered() {
	qCDebug(logStatusNotifierHost) << "Unregistering StatusNotifierItems from old watcher";

	for (auto [service, item]: this->mItems.asKeyValueRange()) {
		emit this->itemUnregistered(item);
		delete item;
		qCDebug(logStatusNotifierHost).noquote()
		    << "Unregistered StatusNotifierItem" << service << "from host";
	}

	this->mItems.clear();
}

void StatusNotifierHost::onItemRegistered(const QString& item) {
	if (this->mItems.contains(item)) {
		qCDebug(logStatusNotifierHost).noquote()
		    << "Ignoring duplicate registration of StatusNotifierItem" << item;
		return;
	}

	qCDebug(logStatusNotifierHost).noquote() << "Registering StatusNotifierItem" << item << "to host";
	auto* dItem = new StatusNotifierItem(item, this);
	if (!dItem->isValid()) {
		qCWarning(logStatusNotifierHost).noquote()
		    << "Unable to connect to StatusNotifierItem at" << item;
		delete dItem;
		return;
	}

	this->mItems.insert(item, dItem);
	QObject::connect(dItem, &StatusNotifierItem::ready, this, &StatusNotifierHost::onItemReady);
	emit this->itemRegistered(dItem);
}

void StatusNotifierHost::onItemUnregistered(const QString& item) {
	if (auto* dItem = this->mItems.value(item)) {
		this->mItems.remove(item);
		emit this->itemUnregistered(dItem);
		delete dItem;
		qCDebug(logStatusNotifierHost).noquote()
		    << "Unregistered StatusNotifierItem" << item << "from host";
	} else {
		qCWarning(logStatusNotifierHost).noquote()
		    << "Ignoring unregistration for missing StatusNotifierItem at" << item;
	}
}

void StatusNotifierHost::onItemReady() {
	if (auto* item = qobject_cast<StatusNotifierItem*>(this->sender())) {
		emit this->itemReady(item);
	}
}

StatusNotifierHost* StatusNotifierHost::instance() {
	static StatusNotifierHost* instance = nullptr; // NOLINT
	if (instance == nullptr) instance = new StatusNotifierHost();
	return instance;
}

} // namespace qs::service::sni
