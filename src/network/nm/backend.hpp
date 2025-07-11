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
#include "device.hpp"

namespace qs::network {

class NetworkManager: public NetworkBackend {
	Q_OBJECT;

public:
	explicit NetworkManager(QObject* parent = nullptr);

	UntypedObjectModel* devices() override;
	NMDevice* wifiDevice() override;
	[[nodiscard]] bool isAvailable() const override;

signals:
	void wifiPoweredChanged();

private slots:
	void onDeviceAdded(const QDBusObjectPath& path);
	void onDeviceRemoved(const QDBusObjectPath& path);

private:
	void init();
	void registerDevice(const QString& path);
	void registerDevices();

	QHash<QString, NMDevice*> mDeviceHash;
	ObjectModel<NMDevice> mDevices {this};
	NMDevice mWifi;

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NetworkManager, dbusProperties);

	DBusNetworkManager* dbus = nullptr;
};

} // namespace qs::network
