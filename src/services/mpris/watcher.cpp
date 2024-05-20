#include "watcher.hpp"

#include <dbus_watcher.h>
#include <qdbusconnection.h>
#include <qdbusconnectioninterface.h>
#include <qdbusservicewatcher.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>

Q_LOGGING_CATEGORY(logMprisWatcher, "quickshell.service.mp.watcher", QtWarningMsg);

namespace qs::service::mp {

MprisWatcher::MprisWatcher(QObject* parent): QObject(parent) {
	new MprisWatcherAdaptor(this);

	qCDebug(logMprisWatcher) << "Starting MprisWatcher";

	auto bus = QDBusConnection::sessionBus();

	if (!bus.isConnected()) {
		qCWarning(logMprisWatcher) << "Could not connect to DBus. Mpris service will not work.";
		return;
	}

	if (!bus.registerObject("/MprisWatcher", this)) {
		qCWarning(logMprisWatcher) << "Could not register MprisWatcher object with "
		                              "DBus. Mpris service will not work.";
		return;
	}

	// clang-format off
	QObject::connect(&this->serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &MprisWatcher::onServiceRegistered);
	QObject::connect(&this->serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &MprisWatcher::onServiceUnregistered);

	this->serviceWatcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration | QDBusServiceWatcher::WatchForRegistration);
	// clang-format on

	this->serviceWatcher.addWatchedService("org.mpris.MediaPlayer2*");
	this->serviceWatcher.addWatchedService("org.mpris.MprisWatcher");
	this->serviceWatcher.setConnection(bus);

	this->tryRegister();
}

void MprisWatcher::tryRegister() { // NOLINT
	auto bus = QDBusConnection::sessionBus();
	auto success = bus.registerService("org.mpris.MprisWatcher");

	if (success) {
		qCDebug(logMprisWatcher) << "Registered watcher at org.mpris.MprisWatcher";
		emit this->MprisWatcherRegistered();
		registerExisting(bus); // Register services that already existed before creation.
	} else {
		qCDebug(logMprisWatcher) << "Could not register watcher at "
		                            "org.mpris.MprisWatcher, presumably because one is "
		                            "already registered.";
		qCDebug(logMprisWatcher
		) << "Registration will be attempted again if the active service is unregistered.";
	}
}

void MprisWatcher::registerExisting(const QDBusConnection& connection) {
	QStringList list = connection.interface()->registeredServiceNames();
	for (const QString& service: list) {
		if (service.contains("org.mpris.MediaPlayer2")) {
			qCDebug(logMprisWatcher).noquote() << "Found Mpris service" << service;
			RegisterMprisPlayer(service);
		}
	}
}

void MprisWatcher::onServiceRegistered(const QString& service) {
	if (service == "org.mpris.MprisWatcher") {
		qCDebug(logMprisWatcher) << "MprisWatcher";
		return;
	} else if (service.contains("org.mpris.MediaPlayer2")) {
		qCDebug(logMprisWatcher).noquote() << "Mpris service " << service << " registered.";
		RegisterMprisPlayer(service);
	} else {
		qCWarning(logMprisWatcher) << "Got a registration event for a untracked service";
	}
}

// TODO: This is getting triggered twice on unregistration, investigate.
void MprisWatcher::onServiceUnregistered(const QString& service) {
	if (service == "org.mpris.MprisWatcher") {
		qCDebug(logMprisWatcher) << "Active MprisWatcher unregistered, attempting registration";
		this->tryRegister();
		return;
	} else {
		QString qualifiedPlayer;
		this->players.removeIf([&](const QString& player) {
			if (QString::compare(player, service) == 0) {
				qualifiedPlayer = player;
				return true;
			} else return false;
		});

		if (!qualifiedPlayer.isEmpty()) {
			qCDebug(logMprisWatcher).noquote()
			    << "Unregistered MprisPlayer" << qualifiedPlayer << "from watcher";

			emit this->MprisPlayerUnregistered(qualifiedPlayer);
		} else {
			qCWarning(logMprisWatcher).noquote()
			    << "Got service unregister event for untracked service" << service;
		}
	}

	this->serviceWatcher.removeWatchedService(service);
}

QList<QString> MprisWatcher::registeredPlayers() const { return this->players; }

void MprisWatcher::RegisterMprisPlayer(const QString& player) {
	if (this->players.contains(player)) {
		qCDebug(logMprisWatcher).noquote()
		    << "Skipping duplicate registration of MprisPlayer" << player << "to watcher";
		return;
	}

	if (!QDBusConnection::sessionBus().interface()->serviceOwner(player).isValid()) {
		qCWarning(logMprisWatcher).noquote()
		    << "Ignoring invalid MprisPlayer registration of" << player << "to watcher";
		return;
	}

	this->serviceWatcher.addWatchedService(player);
	this->players.push_back(player);

	qCDebug(logMprisWatcher).noquote() << "Registered MprisPlayer" << player << "to watcher";

	emit this->MprisPlayerRegistered(player);
}

MprisWatcher* MprisWatcher::instance() {
	static MprisWatcher* instance = nullptr; // NOLINT
	if (instance == nullptr) instance = new MprisWatcher();
	return instance;
}

} // namespace qs::service::mp
