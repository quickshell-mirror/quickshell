#pragma once

#include <qdbusextratypes.h>
#include <qhash.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../wifi.hpp"
#include "accesspoint.hpp"
#include "connection.hpp"
#include "device.hpp"
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

// NMWirelessNetwork aggregates all NMActiveConnection, NMAccessPoint, 
// and NMConnectionSetting objects with the same ssid.
class NMWirelessNetwork: public QObject {
	Q_OBJECT;

public:
	explicit NMWirelessNetwork(QString ssid, QObject* parent = nullptr);
	void addAccessPoint(NMAccessPoint* ap);
	void removeAccessPoint(NMAccessPoint* ap);
	void addConnectionSettings(NMConnectionSettings* conn);
	void removeConnectionSettings(NMConnectionSettings* conn);
	void addActiveConnection(NMActiveConnection* active);
	void removeActiveConnection(NMActiveConnection* active);
	void updateSignalStrength();
	void updateReferenceConnection();

	[[nodiscard]] QString ssid() const { return this->mSsid; };
	[[nodiscard]] quint8 signalStrength() const { return this->mSignalStrength; };
	[[nodiscard]] NMConnectionState::Enum state() const { return this->mState; };
	[[nodiscard]] bool known() const { return this->mKnown; };
	[[nodiscard]] NMConnectionStateReason::Enum reason() const { return this->mReason; };
	[[nodiscard]] NMWirelessSecurityType::Enum security() const { return this->mSecurity; };
	[[nodiscard]] NMAccessPoint* referenceAp() const { return this->mReferenceAp; };
	[[nodiscard]] NMConnectionSettings* referenceConnection() const { return this->mReferenceConn; };
	[[nodiscard]] QList<NMAccessPoint*> accessPoints() const { return this->mAccessPoints.values(); };
	[[nodiscard]] QList<NMConnectionSettings*> connections() const { return this->mConnections.values(); };

public slots:
	void setActiveApPath(const QDBusObjectPath& path);
	void setState(NMConnectionState::Enum state);
	void setReason(NMConnectionStateReason::Enum reason);
	void setKnown(bool known);
	
signals:
	void signalStrengthChanged(quint8 signal);
	void stateChanged(NMConnectionState::Enum state);
	void knownChanged(bool known);
	void reasonChanged(NMConnectionStateReason::Enum reason);
	void securityChanged(NMWirelessSecurityType::Enum security);
	void disappeared();

private:
	QString mSsid;
	QHash<QString, NMAccessPoint*> mAccessPoints;
	QHash<QString, NMConnectionSettings*> mConnections;
	NMActiveConnection* mActiveConnection = nullptr;
	QDBusObjectPath mActiveApPath;
	NMAccessPoint* mReferenceAp = nullptr;
	NMConnectionSettings* mReferenceConn = nullptr;
	NMConnectionState::Enum mState = NMConnectionState::Deactivated;
	NMConnectionStateReason::Enum mReason = NMConnectionStateReason::None;
	NMWirelessSecurityType::Enum mSecurity = NMWirelessSecurityType::None;
	quint8 mSignalStrength = 0;
	bool mKnown = false;
	bool mActive = false;
};

// Proxy of a /org/freedesktop/NetworkManager/Device/* object.
// Extends NMDevice to also include members from the org.freedesktop.NetworkManager.Device.Wireless interface
// Owns the lifetime of NMAccessPoints(s), NMWirelessNetwork(s), frontend WifiNetwork(s).
class NMWirelessDevice: public NMDevice {
	Q_OBJECT;

public:
	explicit NMWirelessDevice(const QString& path, QObject* parent = nullptr);

	[[nodiscard]] bool isWirelessValid() const;
	[[nodiscard]] qint64 getLastScan() { return this->bLastScan; };
	[[nodiscard]] NMWirelessCapabilities::Enum capabilities() { return this->bCapabilities; };
	[[nodiscard]] const QDBusObjectPath& activeApPath() { return this->bActiveAccessPoint; };

public slots:
	void scan();

signals:
	void lastScanChanged(qint64 lastScan);
	void capabilitiesChanged(NMWirelessCapabilities::Enum caps);
	void activeAccessPointChanged(const QDBusObjectPath& path);
	void accessPointLoaded(NMAccessPoint* ap);
	void accessPointRemoved(NMAccessPoint* ap);
	void wifiNetworkAdded(WifiNetwork* net);
	void wifiNetworkRemoved(WifiNetwork* net);
	void activateConnection(const QDBusObjectPath& connPath, const QDBusObjectPath& devPath);
	void addAndActivateConnection(
	    const ConnectionSettingsMap& settings,
	    const QDBusObjectPath& devPath,
	    const QDBusObjectPath& apPath
	);

private slots:
	void onAccessPointPathAdded(const QDBusObjectPath& path);
	void onAccessPointPathRemoved(const QDBusObjectPath& path);
	void onAccessPointLoaded(NMAccessPoint* ap);
	void onConnectionLoaded(NMConnectionSettings* conn);
	void onActiveConnectionLoaded(NMActiveConnection* active);

private:
	void registerAccessPoint(const QString& path);
	void registerAccessPoints();
	void initWireless();
	NMWirelessNetwork* registerNetwork(const QString& ssid);

	QHash<QString, NMAccessPoint*> mAccessPoints;
	QHash<QString, NMWirelessNetwork*> mBackendNetworks;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessDevice, qint64, bLastScan, &NMWirelessDevice::lastScanChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessDevice, NMWirelessCapabilities::Enum, bCapabilities, &NMWirelessDevice::capabilitiesChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessDevice, QDBusObjectPath, bActiveAccessPoint, &NMWirelessDevice::activeAccessPointChanged);
	
	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMWireless, wirelessProperties);
	QS_DBUS_PROPERTY_BINDING(NMWirelessDevice, pLastScan, bLastScan, wirelessProperties, "LastScan");
	QS_DBUS_PROPERTY_BINDING(NMWirelessDevice, pCapabilities, bCapabilities, wirelessProperties, "WirelessCapabilities");
	QS_DBUS_PROPERTY_BINDING(NMWirelessDevice, pActiveAccessPoint, bActiveAccessPoint, wirelessProperties, "ActiveAccessPoint");
	// clang-format on

	DBusNMWirelessProxy* wirelessProxy = nullptr;
};

} // namespace qs::network
