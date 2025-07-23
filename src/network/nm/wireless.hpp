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
#include "accesspoint.hpp"
#include "dbus_nm_wireless.h"

namespace qs::network {

// NMWirelessAdapter wraps the state of a NetworkManager wireless device
// (org.freedesktop.NetworkManager.Device.Wireless), provides signals/slots that connect to a
// frontend NetworkWifiDevice, and creates/destroys NMAccessPointAdapters and NMAccessPointGroups
class NMWirelessAdapter: public QObject {
	Q_OBJECT;

public:
	explicit NMWirelessAdapter(const QString& path, QObject* parent = nullptr);

	void registerAccessPoint(const QString& path);
	void registerAccessPoints();
	void addApToNetwork(NMAccessPointAdapter* ap, const QByteArray& ssid);
	void removeApFromNetwork(NMAccessPointAdapter* ap);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] qint64 getLastScan() { return this->bLastScan; };
	[[nodiscard]] const QDBusObjectPath& activeApPath() { return this->bActiveAccessPoint; };

public slots:
	void scan();

signals:
	void lastScanChanged(qint64 lastScan);
	void activeApChanged(const QDBusObjectPath& path);

	void wifiNetworkAdded(WifiNetwork* network);
	void wifiNetworkRemoved(WifiNetwork* network);

private slots:
	void onAccessPointAdded(const QDBusObjectPath& path);
	void onAccessPointRemoved(const QDBusObjectPath& path);
	void onActiveApChanged(const QDBusObjectPath& path);

private:
	//  Lookups: AP <-> AP group <-> frontend wifi network
	QHash<QString, NMAccessPointAdapter*> mApMap;       // AP Path -> NMAccessPointAdapter*
	QHash<QString, QByteArray> mSsidMap;                // AP Path -> Ssid
	QHash<QByteArray, NMAccessPointGroup*> mApGroupMap; // Ssid -> NMAccessPointGroup*
	QHash<QByteArray, WifiNetwork*> mWifiNetworkMap;    // Ssid -> NetworkWifiNetwork*

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessAdapter, qint64, bLastScan, &NMWirelessAdapter::lastScanChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessAdapter, QDBusObjectPath, bActiveAccessPoint, &NMWirelessAdapter::activeApChanged);
	
	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMWirelessAdapter, wirelessProperties);
	QS_DBUS_PROPERTY_BINDING(NMWirelessAdapter, pLastScan, bLastScan, wirelessProperties, "LastScan");
	QS_DBUS_PROPERTY_BINDING(NMWirelessAdapter, pActiveAccessPoint, bActiveAccessPoint, wirelessProperties, "ActiveAccessPoint");
	// clang-format on

	DBusNMWirelessProxy* proxy = nullptr;
};

} // namespace qs::network
