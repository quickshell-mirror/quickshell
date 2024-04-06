#include "watcher.hpp"

#include <dbus_watcher.h>
#include <qdbusconnection.h>
#include <qdbusconnectioninterface.h>
#include <qdbusservicewatcher.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>

Q_LOGGING_CATEGORY(logStatusNotifierWatcher, "quickshell.service.sni.watcher", QtWarningMsg);

namespace qs::service::sni {

StatusNotifierWatcher::StatusNotifierWatcher(QObject* parent): QObject(parent) {
	new StatusNotifierWatcherAdaptor(this);

	qCDebug(logStatusNotifierWatcher) << "Starting StatusNotifierWatcher";

	auto bus = QDBusConnection::sessionBus();

	if (!bus.isConnected()) {
		qCWarning(logStatusNotifierWatcher)
		    << "Could not connect to DBus. StatusNotifier service will not work.";
		return;
	}

	if (!bus.registerObject("/StatusNotifierWatcher", this)) {
		qCWarning(logStatusNotifierWatcher) << "Could not register StatusNotifierWatcher object with "
		                                       "DBus. StatusNotifer service will not work.";
		return;
	}

	QObject::connect(
	    &this->serviceWatcher,
	    &QDBusServiceWatcher::serviceUnregistered,
	    this,
	    &StatusNotifierWatcher::onServiceUnregistered
	);

	this->serviceWatcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
	this->serviceWatcher.addWatchedService("org.kde.StatusNotifierWatcher");
	this->serviceWatcher.setConnection(bus);

	this->tryRegister();
}

void StatusNotifierWatcher::tryRegister() { // NOLINT
	auto bus = QDBusConnection::sessionBus();
	auto success = bus.registerService("org.kde.StatusNotifierWatcher");

	if (success) {
		qCDebug(logStatusNotifierWatcher) << "Registered watcher at org.kde.StatusNotifierWatcher";
	} else {
		qCDebug(logStatusNotifierWatcher)
		    << "Could not register watcher at org.kde.StatusNotifierWatcher, presumably because one is "
		       "already registered.";
		qCDebug(logStatusNotifierWatcher)
		    << "Registration will be attempted again if the active service is unregistered.";
	}
}

void StatusNotifierWatcher::onServiceUnregistered(const QString& service) {
	if (service == "org.kde.StatusNotifierWatcher") {
		qCDebug(logStatusNotifierWatcher)
		    << "Active StatusNotifierWatcher unregistered, attempting registration";
		this->tryRegister();
		return;
	} else if (this->items.removeAll(service) != 0) {
		qCDebug(logStatusNotifierWatcher).noquote()
		    << "Unregistered StatusNotifierItem" << service << "from watcher";
		emit this->StatusNotifierItemUnregistered(service);
	} else if (this->hosts.removeAll(service) != 0) {
		qCDebug(logStatusNotifierWatcher).noquote()
		    << "Unregistered StatusNotifierHost" << service << "from watcher";
		emit this->StatusNotifierHostUnregistered();
	} else {
		qCWarning(logStatusNotifierWatcher).noquote()
		    << "Got service unregister event for untracked service" << service;
	}

	this->serviceWatcher.removeWatchedService(service);
}

bool StatusNotifierWatcher::isHostRegistered() const { // NOLINT
	// no point ever returning false
	return true;
}

QList<QString> StatusNotifierWatcher::registeredItems() const { return this->items; }

void StatusNotifierWatcher::RegisterStatusNotifierHost(const QString& host) {
	if (this->hosts.contains(host)) {
		qCDebug(logStatusNotifierWatcher).noquote()
		    << "Skipping duplicate registration of StatusNotifierHost" << host << "to watcher";
		return;
	}

	if (!QDBusConnection::sessionBus().interface()->serviceOwner(host).isValid()) {
		qCWarning(logStatusNotifierWatcher).noquote()
		    << "Ignoring invalid StatusNotifierHost registration of" << host << "to watcher";
		return;
	}

	this->serviceWatcher.addWatchedService(host);
	this->hosts.push_back(host);
	qCDebug(logStatusNotifierWatcher).noquote()
	    << "Registered StatusNotifierHost" << host << "to watcher";
	emit this->StatusNotifierHostRegistered();
}

void StatusNotifierWatcher::RegisterStatusNotifierItem(const QString& item) {
	if (this->items.contains(item)) {
		qCDebug(logStatusNotifierWatcher).noquote()
		    << "Skipping duplicate registration of StatusNotifierItem" << item << "to watcher";
		return;
	}

	if (!QDBusConnection::sessionBus().interface()->serviceOwner(item).isValid()) {
		qCWarning(logStatusNotifierWatcher).noquote()
		    << "Ignoring invalid StatusNotifierItem registration of" << item << "to watcher";
		return;
	}

	this->serviceWatcher.addWatchedService(item);
	this->items.push_back(item);
	qCDebug(logStatusNotifierWatcher).noquote()
	    << "Registered StatusNotifierItem" << item << "to watcher";
	emit this->StatusNotifierItemRegistered(item);
}

StatusNotifierWatcher* StatusNotifierWatcher::instance() {
	static StatusNotifierWatcher* instance = nullptr; // NOLINT
	if (instance == nullptr) instance = new StatusNotifierWatcher();
	return instance;
}

} // namespace qs::service::sni
