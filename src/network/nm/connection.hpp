#pragma once

#include <qdbusextratypes.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "../wifi.hpp"
#include "dbus_nm_active_connection.h"
#include "dbus_nm_connection_settings.h"
#include "dbus_types.hpp"
#include "enums.hpp"

namespace qs::dbus {

template <>
struct DBusDataTransform<qs::network::NMConnectionState::Enum> {
	using Wire = quint32;
	using Data = qs::network::NMConnectionState::Enum;
	static DBusResult<Data> fromWire(Wire wire);
};

} // namespace qs::dbus

namespace qs::network {

// Proxy of a /org/freedesktop/NetworkManager/Settings/Connection/* object.
class NMConnectionSettings: public QObject {
	Q_OBJECT;

public:
	explicit NMConnectionSettings(const QString& path, QObject* parent = nullptr);

	void forget();

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] ConnectionSettingsMap settings() const { return this->bSettings; };
	[[nodiscard]] WifiSecurityType::Enum security() const { return this->bSecurity; };
	[[nodiscard]] QBindable<WifiSecurityType::Enum> bindableSecurity() { return &this->bSecurity; };

signals:
	void loaded();
	void settingsChanged(ConnectionSettingsMap settings);
	void securityChanged(WifiSecurityType::Enum security);
	void ssidChanged(QString ssid);

private:
	bool mLoaded = false;
	void updateSettings();
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, ConnectionSettingsMap, bSettings, &NMConnectionSettings::settingsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionSettings, WifiSecurityType::Enum, bSecurity, &NMConnectionSettings::securityChanged);
	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMConnectionSettings, connectionSettingsProperties);
	// clang-format on

	DBusNMConnectionSettingsProxy* proxy = nullptr;
};

// Proxy of a /org/freedesktop/NetworkManager/ActiveConnection/* object.
class NMActiveConnection: public QObject {
	Q_OBJECT;

public:
	explicit NMActiveConnection(const QString& path, QObject* parent = nullptr);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] QDBusObjectPath connection() const { return this->bConnection; };
	[[nodiscard]] NMConnectionState::Enum state() const { return this->bState; };
	[[nodiscard]] NMConnectionStateReason::Enum stateReason() const { return this->mStateReason; };

signals:
	void loaded();
	void connectionChanged(QDBusObjectPath path);
	void stateChanged(NMConnectionState::Enum state);
	void stateReasonChanged(NMConnectionStateReason::Enum reason);
	void uuidChanged(const QString& uuid);

private slots:
	void onStateChanged(quint32 state, quint32 reason);

private:
	NMConnectionStateReason::Enum mStateReason = NMConnectionStateReason::Unknown;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMActiveConnection, QDBusObjectPath, bConnection, &NMActiveConnection::connectionChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMActiveConnection, QString, bUuid, &NMActiveConnection::uuidChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMActiveConnection, NMConnectionState::Enum, bState, &NMActiveConnection::stateChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMActiveConnection, activeConnectionProperties);
	QS_DBUS_PROPERTY_BINDING(NMActiveConnection, pConnection, bConnection, activeConnectionProperties, "Connection");
	QS_DBUS_PROPERTY_BINDING(NMActiveConnection, pUuid, bUuid, activeConnectionProperties, "Uuid");
	QS_DBUS_PROPERTY_BINDING(NMActiveConnection, pState, bState, activeConnectionProperties, "State");
	// clang-format on
	DBusNMActiveConnectionProxy* proxy = nullptr;
};

} // namespace qs::network
