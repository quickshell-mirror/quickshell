#pragma once

#include <qdbusextratypes.h>
#include <qhash.h>
#include <qobject.h>
#include <qproperty.h>
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

template <>
struct DBusDataTransform<QDateTime> {
	using Wire = qint64;
	using Data = QDateTime;
	static DBusResult<Data> fromWire(Wire wire);
};

} // namespace qs::dbus
namespace qs::network {

// NMWirelessNetwork aggregates all related NMActiveConnection, NMAccessPoint, and NMConnectionSetting objects.
class NMWirelessNetwork: public QObject {
	Q_OBJECT;

public:
	explicit NMWirelessNetwork(QString ssid, QObject* parent = nullptr);
	void addAccessPoint(NMAccessPoint* ap);
	void addConnectionSettings(NMConnectionSettings* conn);
	void addActiveConnection(NMActiveConnection* active);

	[[nodiscard]] QString ssid() const { return this->mSsid; };
	[[nodiscard]] quint8 signalStrength() const { return this->bSignalStrength; };
	[[nodiscard]] WifiSecurityType::Enum security() const { return this->bSecurity; };
	[[nodiscard]] NMConnectionState::Enum state() const { return this->bState; };
	[[nodiscard]] bool known() const { return this->bKnown; };
	[[nodiscard]] NMConnectionStateReason::Enum reason() const { return this->bReason; };
	[[nodiscard]] NMAccessPoint* referenceAp() const { return this->mReferenceAp; };
	[[nodiscard]] NMConnectionSettings* referenceConnection() const { return this->mReferenceConn; };
	[[nodiscard]] QList<NMAccessPoint*> accessPoints() const { return this->mAccessPoints.values(); };
	[[nodiscard]] QList<NMConnectionSettings*> connections() const {
		return this->mConnections.values();
	};
	[[nodiscard]] QBindable<QString> bindableActiveApPath() { return &this->bActiveApPath; };
	[[nodiscard]] QBindable<NMWirelessCapabilities::Enum> bindableCapabilities() {
		return &this->bCaps;
	};

signals:
	void signalStrengthChanged(quint8 signal);
	void stateChanged(NMConnectionState::Enum state);
	void knownChanged(bool known);
	void securityChanged(WifiSecurityType::Enum security);
	void reasonChanged(NMConnectionStateReason::Enum reason);
	void capabilitiesChanged(NMWirelessCapabilities::Enum caps);
	void activeApPathChanged(QString path);
	void disappeared();

private:
	void updateReferenceAp();
	void updateReferenceConnection();
	void removeAccessPoint();
	void removeConnectionSettings();
	void removeActiveConnection();

	QString mSsid;
	QHash<QString, NMAccessPoint*> mAccessPoints;
	QHash<QString, NMConnectionSettings*> mConnections;
	NMActiveConnection* mActiveConnection = nullptr;
	NMConnectionSettings* mReferenceConn = nullptr;
	NMAccessPoint* mReferenceAp = nullptr;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, bool, bKnown, &NMWirelessNetwork::knownChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, WifiSecurityType::Enum, bSecurity, &NMWirelessNetwork::securityChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, NMConnectionStateReason::Enum, bReason, &NMWirelessNetwork::reasonChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, NMConnectionState::Enum, bState, &NMWirelessNetwork::stateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, quint8, bSignalStrength, &NMWirelessNetwork::signalStrengthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, NMWirelessCapabilities::Enum, bCaps, &NMWirelessNetwork::capabilitiesChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, QString, bActiveApPath, &NMWirelessNetwork::activeApPathChanged);
	// clang-format on
};

// Proxy of a /org/freedesktop/NetworkManager/Device/* object.
// Extends NMDevice to also include members from the org.freedesktop.NetworkManager.Device.Wireless interface
// Owns the lifetime of NMAccessPoints(s), NMWirelessNetwork(s), frontend WifiNetwork(s).
class NMWirelessDevice: public NMDevice {
	Q_OBJECT;

public:
	explicit NMWirelessDevice(const QString& path, QObject* parent = nullptr);

	[[nodiscard]] bool isWirelessValid() const;
	[[nodiscard]] NMWirelessCapabilities::Enum capabilities() { return this->bCapabilities; };
	[[nodiscard]] const QDBusObjectPath& activeApPath() { return this->bActiveAccessPoint; };

signals:
	void accessPointLoaded(NMAccessPoint* ap);
	void accessPointRemoved(NMAccessPoint* ap);
	void networkAdded(WifiNetwork* net);
	void networkRemoved(WifiNetwork* net);
	void activateConnection(const QDBusObjectPath& connPath, const QDBusObjectPath& devPath);
	void addAndActivateConnection(
	    const ConnectionSettingsMap& settings,
	    const QDBusObjectPath& devPath,
	    const QDBusObjectPath& apPath
	);
	void lastScanChanged(QDateTime lastScan);
	void capabilitiesChanged(NMWirelessCapabilities::Enum caps);
	void activeAccessPointChanged(const QDBusObjectPath& path);

public slots:
	void handleScanner(bool enabled);

private slots:
	void onAccessPointPathAdded(const QDBusObjectPath& path);
	void onAccessPointPathRemoved(const QDBusObjectPath& path);
	void onAccessPointLoaded(NMAccessPoint* ap);
	void onConnectionLoaded(NMConnectionSettings* conn);
	void onActiveConnectionLoaded(NMActiveConnection* active);
	void onScanTimeout();

private:
	void registerAccessPoint(const QString& path);
	void updateNetworkVisibility(WifiNetwork* net);
	void registerAccessPoints();
	void initWireless();
	NMWirelessNetwork* registerNetwork(const QString& ssid);

	QHash<QString, NMAccessPoint*> mAccessPoints;
	QHash<QString, NMWirelessNetwork*> mBackendNetworks;

	QHash<QString, WifiNetwork*> mNetworks;
	QSet<WifiNetwork*> mVisibleNetworks;

	QDateTime mLastScanRequest;
	QTimer mScanTimer;
	bool mScanning = false;
	qint32 mScanIntervalMs = 10001;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessDevice, QDateTime, bLastScan, &NMWirelessDevice::lastScanChanged);
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
