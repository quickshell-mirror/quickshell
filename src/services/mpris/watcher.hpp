#pragma once

#include <qdbuscontext.h>
#include <qdbusinterface.h>
#include <qdbusservicewatcher.h>
#include <qhash.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>

#include "../../core/model.hpp"
#include "player.hpp"

namespace qs::service::mpris {

///! Provides access to MprisPlayers.
class MprisWatcher: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(Mpris);
	QML_SINGLETON;
	/// All connected MPRIS players.
	Q_PROPERTY(ObjectModel<MprisPlayer>* players READ players CONSTANT);

public:
	explicit MprisWatcher(QObject* parent = nullptr);

	[[nodiscard]] ObjectModel<MprisPlayer>* players();

private slots:
	void onServiceRegistered(const QString& service);
	void onServiceUnregistered(const QString& service);
	void onPlayerReady();
	void onPlayerDestroyed(QObject* object);

private:
	void registerExisting();
	void registerPlayer(const QString& address);

	QDBusServiceWatcher serviceWatcher;
	QHash<QString, MprisPlayer*> mPlayers;
	ObjectModel<MprisPlayer> readyPlayers {this};
};

} // namespace qs::service::mpris
