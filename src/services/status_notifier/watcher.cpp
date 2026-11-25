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

#include "../../core/logcat.hpp"

QS_LOGGING_CATEGORY(logStatusNotifierWatcher, "quickshell.service.sni.watcher", QtWarningMsg);

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

	QObject::connect(
	    &this->serviceWatcher,
	    &QDBusServiceWatcher::serviceOwnerChanged,
	    this,
	    &StatusNotifierWatcher::onServiceOwnerChanged
	);

	// WatchForOwnerChange also reports ownership changes that do not pass
	// through an unowned state, which is how a name that is handed over to
	// another owner looks like.
	this->serviceWatcher.setWatchMode(QDBusServiceWatcher::WatchForOwnerChange);
	this->serviceWatcher.addWatchedService("org.kde.StatusNotifierWatcher");
	this->serviceWatcher.setConnection(bus);

	this->tryRegister();
}

bool StatusNotifierWatcher::isRegistered() const { return this->registered; }

void StatusNotifierWatcher::tryRegister() { // NOLINT
	auto bus = QDBusConnection::sessionBus();
	auto success = bus.registerService("org.kde.StatusNotifierWatcher");

	if (success) {
		qCDebug(logStatusNotifierWatcher) << "Registered watcher at org.kde.StatusNotifierWatcher";
		this->registered = true;
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
	} else {
		QString qualifiedItem;
		this->items.removeIf([&](const QString& item) {
			if (item.startsWith(service)) {
				qualifiedItem = item;
				return true;
			} else return false;
		});

		if (!qualifiedItem.isEmpty()) {
			qCDebug(logStatusNotifierWatcher).noquote()
			    << "Unregistered StatusNotifierItem" << qualifiedItem << "from watcher";

			emit this->StatusNotifierItemUnregistered(qualifiedItem);
		} else if (this->hosts.removeAll(service) != 0) {
			qCDebug(logStatusNotifierWatcher).noquote()
			    << "Unregistered StatusNotifierHost" << service << "from watcher";
			emit this->StatusNotifierHostUnregistered();
		} else {
			qCWarning(logStatusNotifierWatcher).noquote()
			    << "Got service unregister event for untracked service" << service;
		}
	}

	this->serviceWatcher.removeWatchedService(service);
}

void StatusNotifierWatcher::onServiceOwnerChanged(
    const QString& service,
    const QString& oldOwner,
    const QString& newOwner
) {
	// This handles one case only: another watcher took the name from us. The
	// old owner has to be us and the new owner has to be somebody else.
	if (service != "org.kde.StatusNotifierWatcher") return;
	if (oldOwner != QDBusConnection::sessionBus().baseService()) return;
	if (oldOwner == newOwner) return;

	// We are not the watcher anymore. The items and hosts we track belong to
	// the watcher that replaced us and are dropped below. tryRegister() will
	// take the name back if it becomes free again.
	qCDebug(logStatusNotifierWatcher) << "StatusNotifierWatcher name taken over by" << newOwner;

	for (auto& item: this->items) {
		this->serviceWatcher.removeWatchedService(item.split("/").at(0));
	}

	for (auto& host: this->hosts) {
		this->serviceWatcher.removeWatchedService(host);
	}

	this->items.clear();
	this->hosts.clear();
	this->registered = false;
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
	auto qualifiedItem = this->qualifiedItem(item);
	auto service = qualifiedItem.split("/").at(0);

	if (this->items.contains(qualifiedItem)) {
		qCDebug(logStatusNotifierWatcher).noquote()
		    << "Skipping duplicate registration of StatusNotifierItem" << qualifiedItem << "to watcher";
		return;
	}

	if (!QDBusConnection::sessionBus().interface()->serviceOwner(service).isValid()) {
		qCWarning(logStatusNotifierWatcher).noquote()
		    << "Ignoring invalid StatusNotifierItem registration of" << qualifiedItem << "to watcher";
		return;
	}

	this->serviceWatcher.addWatchedService(service);
	this->items.push_back(qualifiedItem);

	qCDebug(logStatusNotifierWatcher).noquote()
	    << "Registered StatusNotifierItem" << qualifiedItem << "to watcher";

	emit this->StatusNotifierItemRegistered(qualifiedItem);
}

QString StatusNotifierWatcher::qualifiedItem(const QString& item) {
	// Registered items are often missing either the service id or the path.
	QString qualifiedItem;
	if (item.startsWith("/")) {
		qualifiedItem = this->message().service() + item;
	} else {
		qualifiedItem = item;
	}

	if (!qualifiedItem.contains("/")) {
		qualifiedItem += "/StatusNotifierItem";
	}

	return qualifiedItem;
}

StatusNotifierWatcher* StatusNotifierWatcher::instance() {
	static StatusNotifierWatcher* instance = nullptr; // NOLINT
	if (instance == nullptr) instance = new StatusNotifierWatcher();
	return instance;
}

} // namespace qs::service::sni
