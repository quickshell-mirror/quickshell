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
#include "nm/dbus_nm_accesspoint.h"

namespace qs::network {

class NM80211ApFlags: public QObject {
	Q_OBJECT;

public:
	enum Enum : quint8 {
		None = 0,
		Privacy = 1,
		Wps = 2,
		WpsPbc = 4,
		WpsPin = 8,
	};
	Q_ENUM(Enum);
};

class NM80211ApSecurityFlags: public QObject {
	Q_OBJECT;

public:
	enum Enum : quint16 {
		None = 0,
		PairWep40 = 1,
		PairWep104 = 2,
		PairTkip = 4,
		PairCcmp = 8,
		GroupWep40 = 16,
		GroupWep104 = 32,
		GroupTkip = 64,
		GroupCcmp = 128,
		KeyMgmtPsk = 256,
		KeyMgmt8021x = 512,
		KeyMgmtSae = 1024,
		KeyMgmtOwe = 2048,
		KeyMgmtOweTm = 4096,
		KeyMgmtEapSuiteB192 = 8192,
	};
	Q_ENUM(Enum);
};

} // namespace qs::network

namespace qs::dbus {

template <>
struct DBusDataTransform<qs::network::NM80211ApFlags::Enum> {
	using Wire = quint32;
	using Data = qs::network::NM80211ApFlags::Enum;
	static DBusResult<Data> fromWire(Wire wire);
};

template <>
struct DBusDataTransform<qs::network::NM80211ApSecurityFlags::Enum> {
	using Wire = quint32;
	using Data = qs::network::NM80211ApSecurityFlags::Enum;
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
	void wpaFlagsChanged(NM80211ApSecurityFlags::Enum wpaFlags);
	void rsnFlagsChanged(NM80211ApSecurityFlags::Enum rsnFlags);
	void flagsChanged(NM80211ApFlags::Enum flags);

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPointAdapter, QByteArray, bSsid, &NMAccessPointAdapter::ssidChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPointAdapter, quint8, bSignalStrength, &NMAccessPointAdapter::signalStrengthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPointAdapter, NM80211ApFlags::Enum, bFlags, &NMAccessPointAdapter::flagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPointAdapter, NM80211ApSecurityFlags::Enum, bWpaFlags, &NMAccessPointAdapter::wpaFlagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPointAdapter, NM80211ApSecurityFlags::Enum, bRsnFlags, &NMAccessPointAdapter::rsnFlagsChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMAccessPointAdapter, accessPointProperties);
	QS_DBUS_PROPERTY_BINDING(NMAccessPointAdapter, pSsid, bSsid, accessPointProperties, "Ssid");
	QS_DBUS_PROPERTY_BINDING(NMAccessPointAdapter, pSignalStrength, bSignalStrength, accessPointProperties, "Strength");
	QS_DBUS_PROPERTY_BINDING(NMAccessPointAdapter, pFlags, bFlags, accessPointProperties, "Flags");
	QS_DBUS_PROPERTY_BINDING(NMAccessPointAdapter, pWpaFlags, bWpaFlags, accessPointProperties, "WpaFlags");
	QS_DBUS_PROPERTY_BINDING(NMAccessPointAdapter, pRsnFlags, bRsnFlags, accessPointProperties, "RsnFlags");
	// clang-format on

	DBusNMAccessPointProxy* proxy = nullptr;
};

// NMWifiNetwork represents a wireless network, which aggregates all access points with
// the same SSID. It also provides signals and slots for a frontend WifiNetwork.
class NMWifiNetwork: public QObject {
	Q_OBJECT;

public:
	explicit NMWifiNetwork(QByteArray ssid, QObject* parent = nullptr);
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
	Q_OBJECT_BINDABLE_PROPERTY(NMWifiNetwork, quint8, bMaxSignal, &NMWifiNetwork::signalStrengthChanged);
	// clang-format on
};

} // namespace qs::network
