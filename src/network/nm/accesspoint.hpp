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

// NMAccessPointAdapter wraps the state of a NetworkManager access point
// (org.freedesktop.NetworkManager.AccessPoint)
class NMAccessPointAdapter: public QObject {
	Q_OBJECT;

public:
	explicit NMAccessPointAdapter(const QString& path, QObject* parent = nullptr);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] QByteArray ssid() const { return this->bSsid; };
	[[nodiscard]] quint8 signalStrength() const { return this->bSignalStrength; };
	[[nodiscard]] NM80211ApFlags::Enum flags() const { return this->bFlags; };
	[[nodiscard]] NM80211ApSecurityFlags::Enum wpaFlags() const { return this->bWpaFlags; };
	[[nodiscard]] NM80211ApSecurityFlags::Enum rsnFlags() const { return this->bRsnFlags; };
	[[nodiscard]] NM80211Mode::Enum mode() const { return this->bMode; };

signals:
	void ssidChanged(const QByteArray& ssid);
	void signalStrengthChanged(quint8 signal);
	void wpaFlagsChanged(NM80211ApSecurityFlags::Enum wpaFlags);
	void rsnFlagsChanged(NM80211ApSecurityFlags::Enum rsnFlags);
	void flagsChanged(NM80211ApFlags::Enum flags);
	void modeChanged(NM80211Mode::Enum mode);
	void ready();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPointAdapter, QByteArray, bSsid, &NMAccessPointAdapter::ssidChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPointAdapter, quint8, bSignalStrength, &NMAccessPointAdapter::signalStrengthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPointAdapter, NM80211ApFlags::Enum, bFlags, &NMAccessPointAdapter::flagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPointAdapter, NM80211ApSecurityFlags::Enum, bWpaFlags, &NMAccessPointAdapter::wpaFlagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPointAdapter, NM80211ApSecurityFlags::Enum, bRsnFlags, &NMAccessPointAdapter::rsnFlagsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPointAdapter, NM80211Mode::Enum, bMode, &NMAccessPointAdapter::modeChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMAccessPointAdapter, accessPointProperties);
	QS_DBUS_PROPERTY_BINDING(NMAccessPointAdapter, pSsid, bSsid, accessPointProperties, "Ssid");
	QS_DBUS_PROPERTY_BINDING(NMAccessPointAdapter, pSignalStrength, bSignalStrength, accessPointProperties, "Strength");
	QS_DBUS_PROPERTY_BINDING(NMAccessPointAdapter, pFlags, bFlags, accessPointProperties, "Flags");
	QS_DBUS_PROPERTY_BINDING(NMAccessPointAdapter, pWpaFlags, bWpaFlags, accessPointProperties, "WpaFlags");
	QS_DBUS_PROPERTY_BINDING(NMAccessPointAdapter, pRsnFlags, bRsnFlags, accessPointProperties, "RsnFlags");
	QS_DBUS_PROPERTY_BINDING(NMAccessPointAdapter, pMode, bMode, accessPointProperties, "Mode");
	// clang-format on

	DBusNMAccessPointProxy* proxy = nullptr;
};

} // namespace qs::network
