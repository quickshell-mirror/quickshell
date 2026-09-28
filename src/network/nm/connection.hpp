#pragma once

#include <qbytearray.h>
#include <qdbusextratypes.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qstringlist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "../known_network.hpp"
#include "dbus_nm_connection.h"
#include "dbus_types.hpp"

namespace qs::network {

// Proxy of a /org/freedesktop/NetworkManager/Settings/Connection/* object.
// Owns the lifetime of a frontend KnownNetwork.
class NMConnection: public QObject {
	Q_OBJECT;

public:
	explicit NMConnection(const QString& path, QObject* parent = nullptr);
	void forget();
	QDBusPendingCallWatcher*
	update(const NMSettings& settingsToChange, const NMSettings& settingsToRemove = {});

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] NMSettings settings() { return this->bSettings; }
	[[nodiscard]] KnownNetwork* frontend() { return this->mFrontend; };

signals:
	void loaded();
	// NMDevices and NMNetworks may reference this connection.
	// This signal notifies them before deletion so removal handlers can access path() and frontend().
	void unregistered();
	void settingsChanged(NMSettings settings);

private:
	void getSettings();
	void write(const QVariantMap& settings);
	void bindFrontend();

	KnownNetwork* mFrontend = nullptr;

	Q_OBJECT_BINDABLE_PROPERTY(NMConnection, NMSettings, bSettings, &NMConnection::settingsChanged);
	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMConnection, connectionProperties);
	DBusNMConnectionProxy* proxy = nullptr;
};

} // namespace qs::network
