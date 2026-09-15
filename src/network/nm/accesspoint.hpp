#pragma once

#include <qdbusextratypes.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "../enums.hpp"
#include "dbus_nm_accesspoint.h"
#include "enums.hpp"

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

template <>
struct DBusDataTransform<qs::network::NM80211Mode::Enum> {
	using Wire = quint32;
	using Data = qs::network::NM80211Mode::Enum;
	static DBusResult<Data> fromWire(Wire wire);
};

} // namespace qs::dbus

namespace qs::network {

/// Proxy of a /org/freedesktop/NetworkManager/AccessPoint/* object.
class NMAccessPoint: public QObject {
	Q_OBJECT;

public:
	explicit NMAccessPoint(const QString& path, QObject* parent = nullptr);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] QByteArray ssid() const { return this->bSsid; }
	[[nodiscard]] quint8 signalStrength() const { return this->bSignalStrength; }
	[[nodiscard]] quint32 frequency() const { return this->bFrequency; }
	[[nodiscard]] QString hwAddress() const { return this->bHwAddress; }
	[[nodiscard]] quint32 maxBitrate() const { return this->bMaxBitrate; }
	[[nodiscard]] qint32 lastSeen() const { return this->bLastSeen; }
	[[nodiscard]] quint32 bandwidth() const { return this->bBandwidth; }
	[[nodiscard]] NM80211ApFlags::Enum flags() const { return this->bFlags; }
	[[nodiscard]] NM80211ApSecurityFlags::Enum wpaFlags() const { return this->bWpaFlags; }
	[[nodiscard]] NM80211ApSecurityFlags::Enum rsnFlags() const { return this->bRsnFlags; }
	[[nodiscard]] NM80211Mode::Enum mode() const { return this->bMode; }
	[[nodiscard]] QBindable<WifiSecurityType::Enum> bindableSecurity() { return &this->bSecurity; }
	[[nodiscard]] WifiSecurityType::Enum security() const { return this->bSecurity; }

signals:
	void loaded();
	void ssidChanged(const QByteArray& ssid);
	void signalStrengthChanged(quint8 signal);
	void frequencyChanged(quint32 frequency);
	void hwAddressChanged(const QString& hwAddress);
	void maxBitrateChanged(quint32 maxBitrate);
	void lastSeenChanged(qint32 lastSeen);
	void bandwidthChanged(quint32 bandwidth);
	void flagsChanged(NM80211ApFlags::Enum flags);
	void wpaFlagsChanged(NM80211ApSecurityFlags::Enum wpaFlags);
	void rsnFlagsChanged(NM80211ApSecurityFlags::Enum rsnFlags);
	void modeChanged(NM80211Mode::Enum mode);
	void securityChanged(WifiSecurityType::Enum security);

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPoint, QByteArray, bSsid, &NMAccessPoint::ssidChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPoint, quint8, bSignalStrength, &NMAccessPoint::signalStrengthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPoint, quint32, bFrequency, &NMAccessPoint::frequencyChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPoint, QString, bHwAddress, &NMAccessPoint::hwAddressChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPoint, quint32, bMaxBitrate, &NMAccessPoint::maxBitrateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPoint, qint32, bLastSeen, &NMAccessPoint::lastSeenChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPoint, quint32, bBandwidth, &NMAccessPoint::bandwidthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPoint, NM80211ApFlags::Enum, bFlags, &NMAccessPoint::flagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPoint, NM80211ApSecurityFlags::Enum, bWpaFlags, &NMAccessPoint::wpaFlagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPoint, NM80211ApSecurityFlags::Enum, bRsnFlags, &NMAccessPoint::rsnFlagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPoint, NM80211Mode::Enum, bMode, &NMAccessPoint::modeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPoint, WifiSecurityType::Enum, bSecurity, &NMAccessPoint::securityChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMAccessPointAdapter, accessPointProperties);
	QS_DBUS_PROPERTY_BINDING(NMAccessPoint, pSsid, bSsid, accessPointProperties, "Ssid");
	QS_DBUS_PROPERTY_BINDING(NMAccessPoint, pSignalStrength, bSignalStrength, accessPointProperties, "Strength");
	QS_DBUS_PROPERTY_BINDING(NMAccessPoint, pFrequency, bFrequency, accessPointProperties, "Frequency");
	QS_DBUS_PROPERTY_BINDING(NMAccessPoint, pHwAddress, bHwAddress, accessPointProperties, "HwAddress");
	QS_DBUS_PROPERTY_BINDING(NMAccessPoint, pMaxBitrate, bMaxBitrate, accessPointProperties, "MaxBitrate");
	QS_DBUS_PROPERTY_BINDING(NMAccessPoint, pLastSeen, bLastSeen, accessPointProperties, "LastSeen");
	QS_DBUS_PROPERTY_BINDING(NMAccessPoint, pBandwidth, bBandwidth, accessPointProperties, "Bandwidth");
	QS_DBUS_PROPERTY_BINDING(NMAccessPoint, pFlags, bFlags, accessPointProperties, "Flags");
	QS_DBUS_PROPERTY_BINDING(NMAccessPoint, pWpaFlags, bWpaFlags, accessPointProperties, "WpaFlags");
	QS_DBUS_PROPERTY_BINDING(NMAccessPoint, pRsnFlags, bRsnFlags, accessPointProperties, "RsnFlags");
	QS_DBUS_PROPERTY_BINDING(NMAccessPoint, pMode, bMode, accessPointProperties, "Mode");
	// clang-format on

	DBusNMAccessPointProxy* proxy = nullptr;
};

} // namespace qs::network
