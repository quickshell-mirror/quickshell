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
#include "../api.hpp"
#include "dbus_nm_backend.h"

namespace qs::network {

class NetworkManager: public NetworkBackend {
	Q_OBJECT;

public:
	explicit NetworkManager(QObject* parent = nullptr);

	UntypedObjectModel* devices() override;
	// WirelessDevice* wifiDevice() override;
	[[nodiscard]] bool isAvailable() const override;

private slots:
	void onDeviceAdded(const QDBusObjectPath& path);
	void onDeviceRemoved(const QDBusObjectPath& path);

private:
	void init();
	void registerDevice(const QString& path);
	void registerDevices();

	QHash<QString, Device*> mDeviceHash;
	ObjectModel<Device> mDevices {this};

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NetworkManager, dbusProperties);

	DBusNetworkManagerProxy* proxy = nullptr;
};

} // namespace qs::network
