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

public:
	[[nodiscard]] ObjectModel<MprisPlayer>* players();

	static MprisWatcher* instance();

private slots:
	void onServiceRegistered(const QString& service);
	void onServiceUnregistered(const QString& service);
	void onPlayerReady();
	void onPlayerDestroyed(QObject* object);

private:
	explicit MprisWatcher();

	void registerExisting();
	void registerPlayer(const QString& address);

	QDBusServiceWatcher serviceWatcher;
	QHash<QString, MprisPlayer*> mPlayers;
	ObjectModel<MprisPlayer> readyPlayers {this};
};

class MprisQml: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(Mpris);
	QML_SINGLETON;
	/// All connected MPRIS players.
	Q_PROPERTY(ObjectModel<MprisPlayer>* players READ players CONSTANT);

public:
	explicit MprisQml(QObject* parent = nullptr): QObject(parent) {};

	[[nodiscard]] ObjectModel<MprisPlayer>* players();
};

} // namespace qs::service::mpris
