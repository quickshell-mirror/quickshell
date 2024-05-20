#pragma once

#include <qdbuscontext.h>
#include <qdbusinterface.h>
#include <qdbusservicewatcher.h>
#include <qlist.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtypes.h>

Q_DECLARE_LOGGING_CATEGORY(logMprisWatcher);

namespace qs::service::mp {

class MprisWatcher
    : public QObject
    , protected QDBusContext {
	Q_OBJECT;
	Q_PROPERTY(qint32 ProtocolVersion READ protocolVersion);
	Q_PROPERTY(QList<QString> RegisteredMprisPlayers READ registeredPlayers);

public:
	explicit MprisWatcher(QObject* parent = nullptr);

	void tryRegister();
	void registerExisting(const QDBusConnection &connection); 

	[[nodiscard]] qint32 protocolVersion() const { return 0; } // NOLINT
	[[nodiscard]] QList<QString> registeredPlayers() const;

	// NOLINTBEGIN
	void RegisterMprisPlayer(const QString& player);
	// NOLINTEND

	static MprisWatcher* instance();
	QList<QString> players;

signals:
	// NOLINTBEGIN
	void MprisWatcherRegistered();
	void MprisPlayerRegistered(const QString& service);
	void MprisPlayerUnregistered(const QString& service);	
	// NOLINTEND

private slots:
	void onServiceRegistered(const QString& service); 
	void onServiceUnregistered(const QString& service);

private:	
	QDBusServiceWatcher serviceWatcher;
};

} // namespace qs::service::mp
