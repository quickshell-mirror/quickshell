#pragma once

#include <qdbusextratypes.h>
#include <qhash.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../wifi.hpp"
#include "accesspoint.hpp"
#include "active_connection.hpp"
#include "dbus_nm_wireless.h"
#include "device.hpp"
#include "enums.hpp"
#include "settings.hpp"

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

// NMWirelessNetwork aggregates all related NMActiveConnection, NMAccessPoint, and NMSettings objects.
class NMWirelessNetwork: public QObject {
	Q_OBJECT;

public:
	explicit NMWirelessNetwork(QString ssid, QObject* parent = nullptr);

	void addAccessPoint(NMAccessPoint* ap);
	void addSettings(NMSettings* settings);
	void addActiveConnection(NMActiveConnection* active);
	void forget();

	// clang-format off
	[[nodiscard]] QString ssid() const { return this->mSsid; }
	[[nodiscard]] quint8 signalStrength() const { return this->bSignalStrength; }
	[[nodiscard]] WifiSecurityType::Enum security() const { return this->bSecurity; }
	[[nodiscard]] NMConnectionState::Enum state() const { return this->bState; }
	[[nodiscard]] bool known() const { return this->bKnown; }
	[[nodiscard]] NMConnectionStateReason::Enum reason() const { return this->bReason; }
	QBindable<NMDeviceStateReason::Enum> bindableDeviceFailReason() { return &this->bDeviceFailReason; }
	[[nodiscard]] NMDeviceStateReason::Enum deviceFailReason() const { return this->bDeviceFailReason; }
	[[nodiscard]] NMAccessPoint* referenceAp() const { return this->mReferenceAp; }
	[[nodiscard]] QList<NMAccessPoint*> accessPoints() const { return this->mAccessPoints.values(); }
	[[nodiscard]] QList<NMSettings*> settings() const { return this->mSettings.values(); }
	[[nodiscard]] NMSettings* referenceSettings() const { return this->mReferenceSettings; }
	QBindable<QString> bindableActiveApPath() { return &this->bActiveApPath; }
	QBindable<bool> bindableVisible() { return &this->bVisible; }
	bool visible() const { return this->bVisible; }
	// clang-format on

signals:
	void disappeared();
	void settingsAdded(NMSettings* settings);
	void settingsRemoved(NMSettings* settings);
	void visibilityChanged(bool visible);
	void signalStrengthChanged(quint8 signal);
	void stateChanged(NMConnectionState::Enum state);
	void knownChanged(bool known);
	void securityChanged(WifiSecurityType::Enum security);
	void reasonChanged(NMConnectionStateReason::Enum reason);
	void deviceFailReasonChanged(NMDeviceStateReason::Enum reason);
	void capabilitiesChanged(NMWirelessCapabilities::Enum caps);
	void activeApPathChanged(QString path);

private:
	void updateReferenceAp();
	void updateReferenceSettings();

	QString mSsid;
	QHash<QString, NMAccessPoint*> mAccessPoints;
	QHash<QString, NMSettings*> mSettings;
	NMAccessPoint* mReferenceAp = nullptr;
	NMSettings* mReferenceSettings = nullptr;
	NMActiveConnection* mActiveConnection = nullptr;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, bool, bVisible, &NMWirelessNetwork::visibilityChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, bool, bKnown, &NMWirelessNetwork::knownChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, WifiSecurityType::Enum, bSecurity, &NMWirelessNetwork::securityChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, NMConnectionStateReason::Enum, bReason, &NMWirelessNetwork::reasonChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, NMConnectionState::Enum, bState, &NMWirelessNetwork::stateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMWirelessNetwork, NMDeviceStateReason::Enum, bDeviceFailReason, &NMWirelessNetwork::deviceFailReasonChanged);
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
	[[nodiscard]] NMWirelessCapabilities::Enum capabilities() { return this->bCapabilities; }
	[[nodiscard]] const QDBusObjectPath& activeApPath() { return this->bActiveAccessPoint; }
	[[nodiscard]] NM80211Mode::Enum mode() { return this->bMode; }
	[[nodiscard]] QBindable<bool> bindableScanning() { return &this->bScanning; }

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
	void onSettingsLoaded(NMSettings* settings);
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
