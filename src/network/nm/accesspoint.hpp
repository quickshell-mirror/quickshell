#pragma once

#include <qdbusextratypes.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "enums.hpp"
#include "nm/dbus_nm_accesspoint.h"

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
	explicit NMAccessPoint(
	    const QString& path,
	    NMWirelessCapabilities::Enum caps,
	    QObject* parent = nullptr
	);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] QByteArray ssid() const { return this->bSsid; };
	[[nodiscard]] NMWirelessCapabilities::Enum capabilities() const { return this->mCaps; };
	[[nodiscard]] quint8 signalStrength() const { return this->bSignalStrength; };
	[[nodiscard]] NM80211ApFlags::Enum flags() const { return this->bFlags; };
	[[nodiscard]] NM80211ApSecurityFlags::Enum wpaFlags() const { return this->bWpaFlags; };
	[[nodiscard]] NM80211ApSecurityFlags::Enum rsnFlags() const { return this->bRsnFlags; };
	[[nodiscard]] NM80211Mode::Enum mode() const { return this->bMode; };
	[[nodiscard]] NMWirelessSecurityType::Enum security() const { return this->bSecurity; };

signals:
	void ssidChanged(const QByteArray& ssid);
	void signalStrengthChanged(quint8 signal);
	void wpaFlagsChanged(NM80211ApSecurityFlags::Enum wpaFlags);
	void rsnFlagsChanged(NM80211ApSecurityFlags::Enum rsnFlags);
	void flagsChanged(NM80211ApFlags::Enum flags);
	void securityChanged(NMWirelessSecurityType::Enum security);
	void modeChanged(NM80211Mode::Enum mode);
	void ready();
	void disappeared();

private:
	NMWirelessCapabilities::Enum mCaps;
	bool mActive = false;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPoint, QByteArray, bSsid, &NMAccessPoint::ssidChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPoint, quint8, bSignalStrength, &NMAccessPoint::signalStrengthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPoint, NM80211ApFlags::Enum, bFlags, &NMAccessPoint::flagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPoint, NM80211ApSecurityFlags::Enum, bWpaFlags, &NMAccessPoint::wpaFlagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPoint, NM80211ApSecurityFlags::Enum, bRsnFlags, &NMAccessPoint::rsnFlagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPoint, NM80211Mode::Enum, bMode, &NMAccessPoint::modeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPoint, NMWirelessSecurityType::Enum, bSecurity, &NMAccessPoint::securityChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMAccessPointAdapter, accessPointProperties);
	QS_DBUS_PROPERTY_BINDING(NMAccessPoint, pSsid, bSsid, accessPointProperties, "Ssid");
	QS_DBUS_PROPERTY_BINDING(NMAccessPoint, pSignalStrength, bSignalStrength, accessPointProperties, "Strength");
	QS_DBUS_PROPERTY_BINDING(NMAccessPoint, pFlags, bFlags, accessPointProperties, "Flags");
	QS_DBUS_PROPERTY_BINDING(NMAccessPoint, pWpaFlags, bWpaFlags, accessPointProperties, "WpaFlags");
	QS_DBUS_PROPERTY_BINDING(NMAccessPoint, pRsnFlags, bRsnFlags, accessPointProperties, "RsnFlags");
	QS_DBUS_PROPERTY_BINDING(NMAccessPoint, pMode, bMode, accessPointProperties, "Mode");
	// clang-format on

	DBusNMAccessPointProxy* proxy = nullptr;
};

} // namespace qs::network
