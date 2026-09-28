#pragma once

#include <qdbusextratypes.h>
#include <qhash.h>
#include <qlist.h>
#include <qobject.h>
#include <qproperty.h>
#include <qset.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "../known_network.hpp"
#include "connection.hpp"
#include "dbus_nm_settings.h"

namespace qs::network {

// Proxy of the /org/freedesktop/NetworkManager/Settings object.
// Responsible for the registration of NMConnections.
class NMConnectionManager: public QObject {
	Q_OBJECT;

public:
	explicit NMConnectionManager(QObject* parent = nullptr);
	[[nodiscard]] QList<NMConnection*> loadedConnections() const {
		return this->mLoadedConnections;
	}
	void onServiceRegistered();
	void onServiceUnregistered();

signals:
	void connectionLoaded(NMConnection* conn);

	void knownNetworkAdded(KnownNetwork* knownNet);
	void knownNetworkRemoved(KnownNetwork* knownNet);

private slots:
	void onNewConnectionPath(const QDBusObjectPath& path);
	void onConnectionPathRemoved(const QDBusObjectPath& path);

private:
	void registerConnections();
	void registerConnection(const QString& path);

	QHash<QString, NMConnection*> mConnections;
	QList<NMConnection*> mLoadedConnections;

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMConnectionManager, connectionManagerProperties)
	DBusNMSettingsProxy* proxy = nullptr;
};

} // namespace qs::network
