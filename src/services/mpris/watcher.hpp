#pragma once

#include <qdbuscontext.h>
#include <qdbusinterface.h>
#include <qdbusservicewatcher.h>
#include <qhash.h>
#include <qlist.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "player.hpp"

namespace qs::service::mpris {

///! Provides access to MprisPlayers.
class MprisWatcher: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(Mpris);
	QML_SINGLETON;
	/// All connected MPRIS players.
	Q_PROPERTY(QQmlListProperty<MprisPlayer> players READ players NOTIFY playersChanged);

public:
	explicit MprisWatcher(QObject* parent = nullptr);

	[[nodiscard]] QQmlListProperty<MprisPlayer> players();

signals:
	void playersChanged();

private slots:
	void onServiceRegistered(const QString& service);
	void onServiceUnregistered(const QString& service);
	void onPlayerReady();
	void onPlayerDestroyed(QObject* object);

private:
	static qsizetype playersCount(QQmlListProperty<MprisPlayer>* property);
	static MprisPlayer* playerAt(QQmlListProperty<MprisPlayer>* property, qsizetype index);

	void registerExisting();
	void registerPlayer(const QString& address);

	QDBusServiceWatcher serviceWatcher;
	QHash<QString, MprisPlayer*> mPlayers;
	QList<MprisPlayer*> readyPlayers;
};

} // namespace qs::service::mpris
