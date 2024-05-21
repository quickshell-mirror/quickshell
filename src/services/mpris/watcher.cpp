#include "watcher.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qdbusconnectioninterface.h>
#include <qdbusservicewatcher.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "player.hpp"

namespace qs::service::mpris {

Q_LOGGING_CATEGORY(logMprisWatcher, "quickshell.service.mpris.watcher", QtWarningMsg);

MprisWatcher::MprisWatcher(QObject* parent): QObject(parent) {
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
	this->readyPlayers.push_back(player);
	emit this->playersChanged();
}

void MprisWatcher::onPlayerDestroyed(QObject* object) {
	auto* player = static_cast<MprisPlayer*>(object); // NOLINT

	if (this->readyPlayers.removeOne(player)) {
		emit this->playersChanged();
	}
}

QQmlListProperty<MprisPlayer> MprisWatcher::players() {
	return QQmlListProperty<MprisPlayer>(
	    this,
	    nullptr,
	    &MprisWatcher::playersCount,
	    &MprisWatcher::playerAt
	);
}

qsizetype MprisWatcher::playersCount(QQmlListProperty<MprisPlayer>* property) {
	return static_cast<MprisWatcher*>(property->object)->readyPlayers.count(); // NOLINT
}

MprisPlayer* MprisWatcher::playerAt(QQmlListProperty<MprisPlayer>* property, qsizetype index) {
	return static_cast<MprisWatcher*>(property->object)->readyPlayers.at(index); // NOLINT
}

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

} // namespace qs::service::mpris
