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
#include "dbus_nm_accesspoint.h"

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

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPointAdapter, QByteArray, bSsid, &NMAccessPointAdapter::ssidChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPointAdapter, quint8, bSignalStrength, &NMAccessPointAdapter::signalStrengthChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMAccessPointAdapter, accessPointProperties);
	QS_DBUS_PROPERTY_BINDING(NMAccessPointAdapter, pSsid, bSsid, accessPointProperties, "Ssid");
	QS_DBUS_PROPERTY_BINDING(NMAccessPointAdapter, pSignalStrength, bSignalStrength, accessPointProperties, "Strength");
	// clang-format on

	DBusNMAccessPointProxy* proxy = nullptr;
};

// NMAccessPointGroup bundles the state of access points with the same SSID
// into a single object and supplies signals/slots to connect AP signal strength
// to NetworkWifiNetwork signal strength
class NMAccessPointGroup: public QObject {
	Q_OBJECT;

public:
	explicit NMAccessPointGroup(QByteArray ssid, QObject* parent = nullptr);
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
	Q_OBJECT_BINDABLE_PROPERTY(NMAccessPointGroup, quint8, bMaxSignal, &NMAccessPointGroup::signalStrengthChanged);
	// clang-format on
};

} // namespace qs::network
