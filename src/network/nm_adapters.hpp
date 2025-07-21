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
#include "dbus_nm_accesspoint.h"
#include "dbus_nm_device.h"
#include "dbus_nm_wireless.h"

namespace qs::network {

class NMDeviceState: public QObject {
	Q_OBJECT;

public:
	enum Enum : quint8 {
		Unkown = 0,
		Unmanaged = 10,
		Unavailable = 20,
		Disconnected = 30,
		Prepare = 40,
		Config = 50,
		NeedAuth = 60,
		IPConfig = 70,
		IPCheck = 80,
		Secondaries = 90,
		Activated = 100,
		Deactivating = 110,
		Failed = 120,
	};
	Q_ENUM(Enum);

	static NetworkDeviceState::Enum toNetworkDeviceState(NMDeviceState::Enum state);
};

class NMDeviceType: public QObject {
	Q_OBJECT;

public:
	enum Enum : quint8 {
		Unknown = 0,
		Generic = 14,
		Ethernet = 1,
		Wifi = 2,
		Unused1 = 3,
		Unused2 = 4,
		Bluetooth = 5,
		OlpcMesh = 6,
		Wimax = 7,
		Modem = 8,
		Infiniband = 9,
		Bond = 10,
		Vlan = 11,
		Adsl = 12,
		Bridge = 13,
		Team = 15,
		Tun = 16,
		Tunnel = 17,
		Macvlan = 18,
		Vxlan = 19,
		Veth = 20,
		Macsec = 21,
		Dummy = 22,
		Ppp = 23,
		OvsInterface = 24,
		OvsPort = 25,
		OvsBridge = 26,
		Wpan = 27,
		Lowpan = 28,
		WireGuard = 29,
		WifiP2P = 30,
		Vrf = 31,
		Loopback = 32,
		Hsr = 33,
		Ipvlan = 34,
	};
	Q_ENUM(Enum);
};

} // namespace qs::network

namespace qs::dbus {

template <>
struct DBusDataTransform<qs::network::NMDeviceType::Enum> {
	using Wire = quint32;
	using Data = qs::network::NMDeviceType::Enum;
	static DBusResult<Data> fromWire(Wire wire);
};

template <>
struct DBusDataTransform<qs::network::NMDeviceState::Enum> {
	using Wire = quint32;
	using Data = qs::network::NMDeviceState::Enum;
	static DBusResult<Data> fromWire(Wire wire);
};

} // namespace qs::dbus

namespace qs::network {

// NMAccessPointAdapter wraps the state of a NetworkManager access point
// (org.freedesktop.NetworkManager.AccessPoint)
class NMAccessPointAdapter: public QObject {
	Q_OBJECT;

public:
	explicit NMAccessPointAdapter(const QString& path, QObject* parent = nullptr);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] quint8 getSignal() const { return this->bSignalStrength; };

signals:
	void ssidChanged(const QByteArray& ssid);
	void signalStrengthChanged(quint8 signal);

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPointAdapter, QByteArray, bSsid, &NMAccessPointAdapter::ssidChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPointAdapter, quint8, bSignalStrength, &NMAccessPointAdapter::signalStrengthChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMAccessPointAdapter, accessPointProperties);
	QS_DBUS_PROPERTY_BINDING(NMAccessPointAdapter, pSsid, bSsid, accessPointProperties, "Ssid");
	QS_DBUS_PROPERTY_BINDING(NMAccessPointAdapter, pSignalStrength, bSignalStrength, accessPointProperties, "Strength");
	// clang-format on

	DBusNMAccessPointProxy* proxy = nullptr;
};

// NMDeviceAdapter wraps the state of a NetworkManager device
// (org.freedesktop.NetworkManager.Device) and provides signals/slots
// that connect to a frontend NetworkDevice.
class NMDeviceAdapter: public QObject {
	Q_OBJECT;

public:
	explicit NMDeviceAdapter(const QString& path, QObject* parent = nullptr);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] QString getInterface() { return this->bInterface; };
	[[nodiscard]] QString getHwAddress() { return this->bHwAddress; };
	[[nodiscard]] NMDeviceType::Enum getType() { return this->bType; };

public slots:
	void disconnect();

signals:
	void interfaceChanged(const QString& interface);
	void hwAddressChanged(const QString& hwAddress);
	void typeChanged(NMDeviceType::Enum type);
	void stateChanged(NMDeviceState::Enum state);

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMDeviceAdapter, QString, bInterface, &NMDeviceAdapter::interfaceChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMDeviceAdapter, QString, bHwAddress, &NMDeviceAdapter::hwAddressChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMDeviceAdapter, NMDeviceState::Enum, bState, &NMDeviceAdapter::stateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMDeviceAdapter, NMDeviceType::Enum, bType, &NMDeviceAdapter::typeChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMDeviceAdapter, deviceProperties);
	QS_DBUS_PROPERTY_BINDING(NMDeviceAdapter, pName, bInterface, deviceProperties, "Interface");
	QS_DBUS_PROPERTY_BINDING(NMDeviceAdapter, pAddress, bHwAddress, deviceProperties, "HwAddress");
	QS_DBUS_PROPERTY_BINDING(NMDeviceAdapter, pType, bType, deviceProperties, "DeviceType");
	QS_DBUS_PROPERTY_BINDING(NMDeviceAdapter, pState, bState, deviceProperties, "State");
	// clang-format on

	DBusNMDeviceProxy* proxy = nullptr;
};

// NMAccessPointGroup bundles the state of access points with the same SSID
// into a single object and supplies signals/slots to connect AP signal strength
// to NetworkWifiNetwork signal strength
class NMAccessPointGroup: public QObject {
	Q_OBJECT;

public:
	explicit NMAccessPointGroup(QByteArray ssid, QObject* parent = nullptr);
	void addAccessPoint(NMAccessPointAdapter* ap);
	void removeAccessPoint(NMAccessPointAdapter* ap);
	void updateSignalStrength();
	[[nodiscard]] bool isEmpty() const { return this->mAccessPoints.isEmpty(); };

signals:
	void signalStrengthChanged(quint8 signal);

private:
	QList<NMAccessPointAdapter*> mAccessPoints;
	QByteArray mSsid;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPointGroup, quint8, bMaxSignal, &NMAccessPointGroup::signalStrengthChanged);
	// clang-format on
};

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
