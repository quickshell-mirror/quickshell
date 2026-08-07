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
#include "network.hpp"
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

// Proxy of a /org/freedesktop/NetworkManager/Device/* object.
// Extends NMDevice to also include members from the org.freedesktop.NetworkManager.Device.Wireless interface
// Owns the lifetime of NMAccessPoints(s), NMWirelessNetwork(s), and a frontend WifiDevice.
class NMWirelessDevice: public NMDevice {
	Q_OBJECT;

public:
	explicit NMWirelessDevice(const QString& path, QObject* parent = nullptr);

	[[nodiscard]] bool isValid() const override;
	[[nodiscard]] NMWirelessCapabilities::Enum capabilities() { return this->bCapabilities; }
	[[nodiscard]] const QDBusObjectPath& activeApPath() { return this->bActiveAccessPoint; }
	[[nodiscard]] NM80211Mode::Enum mode() { return this->bMode; }
	[[nodiscard]] QBindable<bool> bindableScanning() { return &this->bScanning; }
	[[nodiscard]] WifiDevice* frontend() override { return this->mFrontend; };

signals:
	void accessPointLoaded(NMAccessPoint* ap);
	void accessPointRemoved(NMAccessPoint* ap);
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
	void removeNetwork();
	bool checkVisibility(WifiNetwork* net);
	void registerAccessPoints();
	void initWireless();
	void bindFrontend();
	NMWirelessNetwork* registerNetwork(const QString& ssid);

	WifiDevice* mFrontend;
	QHash<QString, NMAccessPoint*> mAccessPoints;
	QHash<QString, NMWirelessNetwork*> mNetworks;

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
