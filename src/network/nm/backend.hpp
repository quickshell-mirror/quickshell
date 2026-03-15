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
#include "device.hpp"
#include "enums.hpp"
#include "types.hpp"

namespace qs::network {

class NetworkManager: public NetworkBackend {
	Q_OBJECT;

public:
	explicit NetworkManager(QObject* parent = nullptr);

	[[nodiscard]] bool isAvailable() const override;
	[[nodiscard]] bool wifiEnabled() const { return this->bWifiEnabled; };
	[[nodiscard]] bool wifiHardwareEnabled() const { return this->bWifiHardwareEnabled; };

signals:
	void deviceAdded(NetworkDevice* device);
	void deviceRemoved(NetworkDevice* device);
	void wifiEnabledChanged(bool enabled);
	void wifiHardwareEnabledChanged(bool enabled);

public slots:
	void setWifiEnabled(bool enabled);

private slots:
	void onDevicePathAdded(const QDBusObjectPath& path);
	void onDevicePathRemoved(const QDBusObjectPath& path);
	void activateConnection(const QDBusObjectPath& connPath, const QDBusObjectPath& devPath);
	void addAndActivateConnection(
	    const ConnectionSettingsMap& settings,
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

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NetworkManager, dbusProperties);
	QS_DBUS_PROPERTY_BINDING(NetworkManager, pWifiEnabled, bWifiEnabled, dbusProperties, "WirelessEnabled");
	QS_DBUS_PROPERTY_BINDING(NetworkManager, pWifiHardwareEnabled, bWifiHardwareEnabled, dbusProperties, "WirelessHardwareEnabled");
	// clang-format on
	DBusNetworkManagerProxy* proxy = nullptr;
};

} // namespace qs::network
