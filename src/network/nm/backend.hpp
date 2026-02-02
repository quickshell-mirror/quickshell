#pragma once

#include <qdbusextratypes.h>
#include <qhash.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "../network.hpp"
#include "dbus_nm_backend.h"
#include "dbus_types.hpp"
#include "device.hpp"
#include "enums.hpp"

namespace qs::dbus {

template <>
struct DBusDataTransform<qs::network::NMConnectivityState::Enum> {
	using Wire = quint32;
	using Data = qs::network::NMConnectivityState::Enum;
	static DBusResult<Data> fromWire(Wire wire);
};

} // namespace qs::dbus

namespace qs::network {

class NetworkManager: public NetworkBackend {
	Q_OBJECT;

public:
	explicit NetworkManager(QObject* parent = nullptr);

	[[nodiscard]] bool isAvailable() const override;
	[[nodiscard]] bool wifiEnabled() const { return this->bWifiEnabled; }
	[[nodiscard]] bool wifiHardwareEnabled() const { return this->bWifiHardwareEnabled; }
	[[nodiscard]] bool connectivityCheckAvailable() const {
		return this->bConnectivityCheckAvailable;
	};
	[[nodiscard]] bool connectivityCheckEnabled() const { return this->bConnectivityCheckEnabled; }
	[[nodiscard]] NMConnectivityState::Enum connectivity() const { return this->bConnectivity; }

signals:
	void deviceAdded(NetworkDevice* device);
	void deviceRemoved(NetworkDevice* device);
	void wifiEnabledChanged(bool enabled);
	void wifiHardwareEnabledChanged(bool enabled);
	void connectivityStateChanged(NMConnectivityState::Enum state);
	void connectivityCheckAvailableChanged(bool available);
	void connectivityCheckEnabledChanged(bool enabled);

public slots:
	void setWifiEnabled(bool enabled);
	void setConnectivityCheckEnabled(bool enabled);
	void checkConnectivity();

private slots:
	void onDevicePathAdded(const QDBusObjectPath& path);
	void onDevicePathRemoved(const QDBusObjectPath& path);
	void activateConnection(const QDBusObjectPath& connPath, const QDBusObjectPath& devPath);
	void addAndActivateConnection(
	    const NMSettingsMap& settings,
	    const QDBusObjectPath& devPath,
	    const QDBusObjectPath& specificObjectPath
	);

private:
	void init();
	void registerDevices();
	void registerDevice(const QString& path);
	void registerFrontendDevice(NMDeviceType::Enum type, NMDevice* dev);
	void removeFrontendDevice(NMDevice* dev);

	QHash<QString, NMDevice*> mDevices;
	QHash<QString, NetworkDevice*> mFrontendDevices;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NetworkManager, bool, bWifiEnabled, &NetworkManager::wifiEnabledChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkManager, bool, bWifiHardwareEnabled, &NetworkManager::wifiHardwareEnabledChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkManager, NMConnectivityState::Enum, bConnectivity, &NetworkManager::connectivityStateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkManager, bool, bConnectivityCheckAvailable, &NetworkManager::connectivityCheckAvailableChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkManager, bool, bConnectivityCheckEnabled, &NetworkManager::connectivityCheckEnabledChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NetworkManager, dbusProperties);
	QS_DBUS_PROPERTY_BINDING(NetworkManager, pWifiEnabled, bWifiEnabled, dbusProperties, "WirelessEnabled");
	QS_DBUS_PROPERTY_BINDING(NetworkManager, pWifiHardwareEnabled, bWifiHardwareEnabled, dbusProperties, "WirelessHardwareEnabled");
	QS_DBUS_PROPERTY_BINDING(NetworkManager, pConnectivity, bConnectivity, dbusProperties, "Connectivity");
	QS_DBUS_PROPERTY_BINDING(NetworkManager, pConnectivityCheckAvailable, bConnectivityCheckAvailable, dbusProperties, "ConnectivityCheckAvailable");
	QS_DBUS_PROPERTY_BINDING(NetworkManager, pConnectivityCheckEnabled, bConnectivityCheckEnabled, dbusProperties, "ConnectivityCheckEnabled");
	// clang-format on
	DBusNetworkManagerProxy* proxy = nullptr;
};

} // namespace qs::network
