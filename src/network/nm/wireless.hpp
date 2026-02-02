#pragma once

#include <qdbusextratypes.h>
#include <qhash.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../wifi.hpp"
#include "accesspoint.hpp"
#include "dbus_nm_wireless.h"
#include "device.hpp"
#include "enums.hpp"

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
	void addConnection(NMConnectionSettings* conn);
	void registerFrontendConnection(NMConnectionSettings* conn);
	void removeFrontendConnection(NMConnectionSettings* conn);
	void addActiveConnection(NMActiveConnection* active);
	void setDefaultConnection(NMConnection* frontendConn);
	void forget();

	[[nodiscard]] QString ssid() const { return this->mSsid; };
	[[nodiscard]] quint8 signalStrength() const { return this->bSignalStrength; };
	[[nodiscard]] WifiSecurityType::Enum security() const { return this->bSecurity; };
	[[nodiscard]] NMConnectionState::Enum state() const { return this->bState; };
	[[nodiscard]] bool known() const { return this->bKnown; };
	[[nodiscard]] NMNetworkStateReason::Enum reason() const { return this->bReason; };
	[[nodiscard]] NMAccessPoint* referenceAp() const { return this->mReferenceAp; };
	[[nodiscard]] QList<NMAccessPoint*> accessPoints() const { return this->mAccessPoints.values(); };
	[[nodiscard]] NMConnectionSettings* defaultConn() const { return this->mDefaultConnection; };
	[[nodiscard]] NMConnection* defaultFrontendConn() const {
		return this->bDefaultFrontendConnection;
	};
	[[nodiscard]] QList<NMConnectionSettings*> connections() const {
		return this->mConnections.values();
	}
	[[nodiscard]] QList<NMConnection*> frontendConnections() const {
		return this->mFrontendConnections.values();
	}
	[[nodiscard]] QBindable<QString> bindableActiveApPath() { return &this->bActiveApPath; };
	[[nodiscard]] QBindable<bool> bindableVisible() { return &this->bVisible; };
	[[nodiscard]] bool visible() const { return this->bVisible; };

signals:
	void disappeared();
	void connectionAdded(NMConnection* conn);
	void connectionRemoved(NMConnection* conn);
	void defaultFrontendConnectionChanged(NMConnection* conn);
	void visibilityChanged(bool visible);
	void signalStrengthChanged(quint8 signal);
	void stateChanged(NMConnectionState::Enum state);
	void knownChanged(bool known);
	void securityChanged(WifiSecurityType::Enum security);
	void reasonChanged(NMNetworkStateReason::Enum reason);
	void capabilitiesChanged(NMWirelessCapabilities::Enum caps);
	void activeApPathChanged(QString path);

private:
	void updateReferenceAp();
	void updateDefaultConnection();

	QString mSsid;
	QHash<QString, NMAccessPoint*> mAccessPoints;
	QHash<QString, NMConnectionSettings*> mConnections;
	QHash<QString, NMConnection*> mFrontendConnections;
	NMAccessPoint* mReferenceAp = nullptr;
	NMActiveConnection* mActiveConnection = nullptr;
	NMConnectionSettings* mDefaultConnection = nullptr;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, NMConnection*, bDefaultFrontendConnection, &NMWirelessNetwork::defaultFrontendConnectionChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, bool, bVisible, &NMWirelessNetwork::visibilityChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, bool, bKnown, &NMWirelessNetwork::knownChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, WifiSecurityType::Enum, bSecurity, &NMWirelessNetwork::securityChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, NMNetworkStateReason::Enum, bReason, &NMWirelessNetwork::reasonChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, NMConnectionState::Enum, bState, &NMWirelessNetwork::stateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, quint8, bSignalStrength, &NMWirelessNetwork::signalStrengthChanged);
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

	[[nodiscard]] bool isValid() const override;
	[[nodiscard]] NMWirelessCapabilities::Enum capabilities() { return this->bCapabilities; };
	[[nodiscard]] const QDBusObjectPath& activeApPath() { return this->bActiveAccessPoint; };
	[[nodiscard]] NM80211Mode::Enum mode() { return this->bMode; };
	[[nodiscard]] QBindable<bool> bindableScanning() { return &this->bScanning; };

signals:
	void accessPointLoaded(NMAccessPoint* ap);
	void accessPointRemoved(NMAccessPoint* ap);
	void networkAdded(WifiNetwork* net);
	void networkRemoved(WifiNetwork* net);
	void lastScanChanged(QDateTime lastScan);
	void scanningChanged(bool scanning);
	void capabilitiesChanged(NMWirelessCapabilities::Enum caps);
	void activeAccessPointChanged(const QDBusObjectPath& path);
	void modeChanged(NM80211Mode::Enum mode);

private slots:
	void onAccessPointAdded(const QDBusObjectPath& path);
	void onAccessPointRemoved(const QDBusObjectPath& path);
	void onAccessPointLoaded(NMAccessPoint* ap);
	void onConnectionLoaded(NMConnectionSettings* conn);
	void onActiveConnectionLoaded(NMActiveConnection* active);
	void onScanTimeout();
	void onScanningChanged(bool scanning);

private:
	void registerAccessPoint(const QString& path);
	void registerFrontendNetwork(NMWirelessNetwork* net);
	void removeFrontendNetwork(NMWirelessNetwork* net);
	void removeNetwork();
	bool checkVisibility(WifiNetwork* net);
	void registerAccessPoints();
	void initWireless();
	NMWirelessNetwork* registerNetwork(const QString& ssid);

	QHash<QString, NMAccessPoint*> mAccessPoints;
	QHash<QString, NMWirelessNetwork*> mNetworks;
	QHash<QString, WifiNetwork*> mFrontendNetworks;

	QDateTime mLastScanRequest;
	QTimer mScanTimer;
	qint32 mScanIntervalMs = 10001;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessDevice, bool, bScanning, &NMWirelessDevice::scanningChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessDevice, QDateTime, bLastScan, &NMWirelessDevice::lastScanChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessDevice, NMWirelessCapabilities::Enum, bCapabilities, &NMWirelessDevice::capabilitiesChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessDevice, QDBusObjectPath, bActiveAccessPoint, &NMWirelessDevice::activeAccessPointChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessDevice, NM80211Mode::Enum, bMode, &NMWirelessDevice::modeChanged);
	
	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMWireless, wirelessProperties);
	QS_DBUS_PROPERTY_BINDING(NMWirelessDevice, pLastScan, bLastScan, wirelessProperties, "LastScan");
	QS_DBUS_PROPERTY_BINDING(NMWirelessDevice, pCapabilities, bCapabilities, wirelessProperties, "WirelessCapabilities");
	QS_DBUS_PROPERTY_BINDING(NMWirelessDevice, pActiveAccessPoint, bActiveAccessPoint, wirelessProperties, "ActiveAccessPoint");
	QS_DBUS_PROPERTY_BINDING(NMWirelessDevice, pMode, bMode, wirelessProperties, "Mode");
	// clang-format on

	DBusNMWirelessProxy* wirelessProxy = nullptr;
};

} // namespace qs::network
