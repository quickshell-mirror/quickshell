#include "watcher.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qdbusconnectioninterface.h>
#include <qdbusservicewatcher.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qqmllist.h>

#include "../../core/logcat.hpp"
#include "../../core/model.hpp"
#include "player.hpp"

namespace qs::service::mpris {

namespace {
QS_LOGGING_CATEGORY(logMprisWatcher, "quickshell.service.mpris.watcher", QtWarningMsg);
}

MprisWatcher::MprisWatcher() {
	qCDebug(logMprisWatcher) << "Starting MprisWatcher";

	auto bus = QDBusConnection::sessionBus();

	if (!bus.isConnected()) {
		qCWarning(logMprisWatcher) << "Could not connect to DBus. Mpris service will not work.";
		return;
	}

	// clang-format off
	QObject::connect(&this->serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &MprisWatcher::onServiceRegistered);
	QObject::connect(&this->serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &MprisWatcher::onServiceUnregistered);
	// clang-format on

	this->serviceWatcher.setWatchMode(
	    QDBusServiceWatcher::WatchForUnregistration | QDBusServiceWatcher::WatchForRegistration
	);

	this->serviceWatcher.addWatchedService("org.mpris.MediaPlayer2*");
	this->serviceWatcher.setConnection(bus);

	this->registerExisting();
}

void MprisWatcher::registerExisting() {
	const QStringList& list = QDBusConnection::sessionBus().interface()->registeredServiceNames();
	for (const QString& service: list) {
		if (service.startsWith("org.mpris.MediaPlayer2")) {
			qCDebug(logMprisWatcher).noquote() << "Found Mpris service" << service;
			this->registerPlayer(service);
		}
	}
}

void MprisWatcher::onServiceRegistered(const QString& service) {
	if (service.startsWith("org.mpris.MediaPlayer2")) {
		qCDebug(logMprisWatcher).noquote() << "Mpris service " << service << " registered.";
		this->registerPlayer(service);
	} else {
		qCWarning(logMprisWatcher) << "Got a registration event for untracked service" << service;
	}
}

void MprisWatcher::onServiceUnregistered(const QString& service) {
	if (auto* player = this->mPlayers.value(service)) {
		player->deleteLater();
		this->mPlayers.remove(service);
		qCDebug(logMprisWatcher) << "Unregistered MprisPlayer" << service;
	} else {
		qCWarning(logMprisWatcher) << "Got service unregister event for untracked service" << service;
	}
}

void MprisWatcher::onPlayerReady() {
	auto* player = qobject_cast<MprisPlayer*>(this->sender());
	this->readyPlayers.insertObject(player);
}

void MprisWatcher::onPlayerDestroyed(QObject* object) {
	auto* player = static_cast<MprisPlayer*>(object); // NOLINT
	this->readyPlayers.removeObject(player);
}

ObjectModel<MprisPlayer>* MprisWatcher::players() { return &this->readyPlayers; }

void MprisWatcher::registerPlayer(const QString& address) {
	if (this->mPlayers.contains(address)) {
		qCDebug(logMprisWatcher) << "Skipping duplicate registration of MprisPlayer" << address;
		return;
	}

	auto* player = new MprisPlayer(address, this);
	if (!player->isValid()) {
		qCWarning(logMprisWatcher) << "Ignoring invalid MprisPlayer registration of" << address;
		delete player;
		return;
	}

	this->mPlayers.insert(address, player);
	QObject::connect(player, &MprisPlayer::ready, this, &MprisWatcher::onPlayerReady);
	QObject::connect(player, &QObject::destroyed, this, &MprisWatcher::onPlayerDestroyed);

	qCDebug(logMprisWatcher) << "Registered MprisPlayer" << address;
}

MprisWatcher* MprisWatcher::instance() {
	static MprisWatcher* instance = new MprisWatcher(); // NOLINT
	return instance;
}

ObjectModel<MprisPlayer>* MprisQml::players() { // NOLINT
	return MprisWatcher::instance()->players();
}

} // namespace qs::service::mpris
