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

#include "../dbus/properties.hpp"
#include "api.hpp"
#include "dbus_nm_backend.h"
#include "nm_adapters.hpp"

namespace qs::network {

class NetworkManager: public NetworkBackend {
	Q_OBJECT;

public:
	explicit NetworkManager(QObject* parent = nullptr);

	UntypedObjectModel* devices() override;
	WirelessNetworkDevice* defaultWifiDevice() override;
	[[nodiscard]] bool isAvailable() const override;

private slots:
	void onDeviceAdded(const QDBusObjectPath& path);
	void onDeviceRemoved(const QDBusObjectPath& path);

private:
	void init();
	void registerDevice(NMDeviceAdapter* deviceAdapter, NMDeviceType::Enum type, const QString& path);
	void registerDevices();
	void queueDeviceRegistration(const QString& path);
	NetworkDevice* createDeviceVariant(NMDeviceType::Enum type, const QString& path);
	WirelessNetworkDevice* bindWirelessDevice(const QString& path);
	NetworkDevice* bindDevice(NMDeviceAdapter* deviceAdapter);

	QHash<QString, NetworkDevice*> mDeviceHash;
	ObjectModel<NetworkDevice> mDevices {this};
	WirelessNetworkDevice* mWifi = nullptr;

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NetworkManager, dbusProperties);

	DBusNetworkManagerProxy* proxy = nullptr;
};

} // namespace qs::network
