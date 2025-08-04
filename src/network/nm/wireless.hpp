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

#include "../frontend.hpp"
#include "accesspoint.hpp"
#include "connection.hpp"
#include "enums.hpp"
#include "nm/dbus_nm_wireless.h"

namespace qs::dbus {
template <>
struct DBusDataTransform<qs::network::NMWirelessCapabilities::Enum> {
	using Wire = quint32;
	using Data = qs::network::NMWirelessCapabilities::Enum;
	static DBusResult<Data> fromWire(Wire wire);
};

} // namespace qs::dbus
namespace qs::network {

// NMWirelessNetwork represents a wireless network, which aggregates all access points with
// the same SSID. It also provides signals and slots for a frontend WifiNetwork.
class NMWirelessNetwork: public QObject {
	Q_OBJECT;

public:
	explicit NMWirelessNetwork(
	    NMWirelessCapabilities::Enum caps,
	    QByteArray ssid,
	    QObject* parent = nullptr
	);
	void addAccessPoint(NMAccessPointAdapter* ap);
	void removeAccessPoint(NMAccessPointAdapter* ap);
	void updateSignalStrength();

	[[nodiscard]] bool isEmpty() const { return this->mAccessPoints.isEmpty(); };
	[[nodiscard]] quint8 signalStrength() const { return this->bMaxSignal; };
	[[nodiscard]] QString ssid() const { return this->mSsid; };
	[[nodiscard]] NMAccessPointAdapter* referenceAccessPoint() const { return this->mReferenceAP; };
	[[nodiscard]] NMWirelessSecurityType::Enum findBestSecurity();

signals:
	void signalStrengthChanged(quint8 signal);
	void referenceAccessPointChanged(NMAccessPointAdapter* ap);
	void disappeared(const QString& ssid);

private:
	NMAccessPointAdapter* mReferenceAP = nullptr;
	QList<NMAccessPointAdapter*> mAccessPoints;
	NMWirelessCapabilities::Enum mDeviceCapabilities;
	QByteArray mSsid;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, quint8, bMaxSignal, &NMWirelessNetwork::signalStrengthChanged);
	// clang-format on
};

// NMWirelessAdapter wraps the state of a NetworkManager wireless device
// (org.freedesktop.NetworkManager.Device.Wireless), provides signals/slots that connect to a
// frontend NetworkWifiDevice, and creates/destroys NMAccessPointAdapters and NMAccessPointGroups
class NMWirelessAdapter: public QObject {
	Q_OBJECT;

public:
	explicit NMWirelessAdapter(const QString& path, QObject* parent = nullptr);

	void init();
	void registerAccessPoint(const QString& path);
	void registerAccessPoints();
	void addApToNetwork(NMAccessPointAdapter* ap, const QByteArray& ssid);
	void removeApFromNetwork(NMAccessPointAdapter* ap);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] qint64 getLastScan() { return this->bLastScan; };
	[[nodiscard]] const QDBusObjectPath& activeApPath() { return this->bActiveAccessPoint; };
	[[nodiscard]] NMWirelessCapabilities::Enum capabilities() { return this->bCapabilities; };

public slots:
	void scan();

signals:
	void lastScanChanged(qint64 lastScan);
	void activeApChanged(const QDBusObjectPath& path);
	void capabilitiesChanged(NMWirelessCapabilities::Enum caps);

	void networkAdded(NMWirelessNetwork* network);
	void networkRemoved(NMWirelessNetwork* network);

private slots:
	void onAccessPointAdded(const QDBusObjectPath& path);
	void onAccessPointRemoved(const QDBusObjectPath& path);
	void onAccessPointReady(NMAccessPointAdapter* ap);

private:
	//  Lookups: NMAccessPointAdapter <-> NMWifiNetwork
	QHash<QString, NMAccessPointAdapter*> mApMap;      // AP Path -> NMAccessPointAdapter*
	QHash<QString, QByteArray> mSsidMap;               // AP Path -> Ssid
	QHash<QByteArray, NMWirelessNetwork*> mNetworkMap; // Ssid -> NMAccessPointGroup*

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessAdapter, qint64, bLastScan, &NMWirelessAdapter::lastScanChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessAdapter, QDBusObjectPath, bActiveAccessPoint, &NMWirelessAdapter::activeApChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessAdapter, NMWirelessCapabilities::Enum, bCapabilities, &NMWirelessAdapter::capabilitiesChanged);
	
	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMWirelessAdapter, wirelessProperties);
	QS_DBUS_PROPERTY_BINDING(NMWirelessAdapter, pLastScan, bLastScan, wirelessProperties, "LastScan");
	QS_DBUS_PROPERTY_BINDING(NMWirelessAdapter, pActiveAccessPoint, bActiveAccessPoint, wirelessProperties, "ActiveAccessPoint");
	QS_DBUS_PROPERTY_BINDING(NMWirelessAdapter, pCapabilities, bCapabilities, wirelessProperties, "WirelessCapabilities");
	// clang-format on

	DBusNMWirelessProxy* proxy = nullptr;
};

// NMWirelessManager processes the incoming/outgoing connections (NMDeviceAdapter) and wifi
// networks (NMWirelessAdapter), and merges them into WifiItems for the frontend.
class NMWirelessManager: public QObject {
	Q_OBJECT;

public:
	explicit NMWirelessManager(QObject* parent = nullptr);

public slots:
	void connectionLoaded(NMConnectionSettingsAdapter* connection);
	void connectionRemoved(NMConnectionSettingsAdapter* connection);
	void networkAdded(NMWirelessNetwork* nmNetwork);
	void networkRemoved(NMWirelessNetwork* nmNetwork);

signals:
	void wifiNetworkAdded(WifiNetwork* network);
	void wifiNetworkRemoved(WifiNetwork* network);

private:
	// Lookups to help merge 1:1 connection:item relation and 1:many network:item relation.
	QHash<QString, NMWirelessNetwork*> mNetworkMap;      // Ssid -> NMWirelessNetwork
	QHash<QString, NMConnectionSettingsAdapter*> mConnectionMap; // Uuid -> NMConnectionAdapter
	QHash<QString, WifiNetwork*> mUuidToItem;            // Uuid -> WifiNetwork
	QMultiHash<QString, WifiNetwork*> mSsidToItem;       // Ssid -> WifiNetwork
};

} // namespace qs::network
