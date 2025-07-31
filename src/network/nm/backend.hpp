#pragma once

#include <qcontainerfwd.h>
#include <qdbusextratypes.h>
#include <qdbusservicewatcher.h>
#include <qhash.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "../frontend.hpp"
#include "device.hpp"
#include "nm/dbus_nm_backend.h"

namespace qs::network {

class NetworkManager: public NetworkBackend {
	Q_OBJECT;

signals:
	void deviceAdded(NetworkDevice* device);
	void deviceRemoved(NetworkDevice* device);

public:
	explicit NetworkManager(QObject* parent = nullptr);
	[[nodiscard]] bool isAvailable() const override;

private slots:
	void onDeviceAdded(const QDBusObjectPath& path);
	void onDeviceRemoved(const QDBusObjectPath& path);

private:
	void init();
	void registerDevice(NMDeviceAdapter* deviceAdapter, const QString& path);
	void registerDevices();
	void queueDeviceRegistration(const QString& path);
	static NetworkDeviceState::Enum toNetworkDeviceState(NMDeviceState::Enum state);
	NetworkWifiDevice* bindWirelessDevice(NMDeviceAdapter* deviceAdapter, const QString& path);

	QHash<QString, NetworkDevice*> mDeviceHash;

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NetworkManager, dbusProperties);
	DBusNetworkManagerProxy* proxy = nullptr;
};

} // namespace qs::network
