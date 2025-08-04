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
#include "connection.hpp"
#include "enums.hpp"
#include "nm/dbus_nm_device.h"

namespace qs::dbus {

template <>
struct DBusDataTransform<qs::network::NMDeviceType::Enum> {
	using Wire = quint32;
	using Data = qs::network::NMDeviceType::Enum;
	static DBusResult<Data> fromWire(Wire wire);
};

template <>
struct DBusDataTransform<qs::network::NMDeviceState::Enum> {
	using Wire = quint32;
	using Data = qs::network::NMDeviceState::Enum;
	static DBusResult<Data> fromWire(Wire wire);
};

} // namespace qs::dbus

namespace qs::network {

// NMDeviceAdapter wraps the state of a NetworkManager device
// (org.freedesktop.NetworkManager.Device) and provides signals/slots
// that connect to a frontend NetworkDevice.
class NMDeviceAdapter: public QObject {
	Q_OBJECT;

public:
	explicit NMDeviceAdapter(const QString& path, QObject* parent = nullptr);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] QString interface() { return this->bInterface; };
	[[nodiscard]] QString hwAddress() { return this->bHwAddress; };
	[[nodiscard]] NMDeviceType::Enum type() { return this->bType; };
	[[nodiscard]] NMDeviceState::Enum state() { return this->bState; };

public slots:
	void disconnect();

signals:
	void ready();
	void connectionLoaded(NMConnectionSettingsAdapter* connection);
	void connectionRemoved(NMConnectionSettingsAdapter* connection);
	void activeConnectionLoaded(NMActiveConnectionAdapter* connection);
	void activeConnectionRemoved(NMActiveConnectionAdapter* connection);
	void interfaceChanged(const QString& interface);
	void hwAddressChanged(const QString& hwAddress);
	void typeChanged(NMDeviceType::Enum type);
	void stateChanged(NMDeviceState::Enum state);
	void availableConnectionsChanged(QList<QDBusObjectPath> paths);
	void activeConnectionChanged(const QDBusObjectPath& connection);

private slots:
	void onAvailableConnectionsChanged(const QList<QDBusObjectPath>& paths);
	void onActiveConnectionChanged(const QDBusObjectPath& path);

private:
	void registerConnection(const QString& path);
	// Connection lookups
	QSet<QString> mConnectionPaths;
	QHash<QString, NMConnectionSettingsAdapter*> mConnectionMap;
	NMActiveConnectionAdapter* mActiveConnection = nullptr;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMDeviceAdapter, QString, bInterface, &NMDeviceAdapter::interfaceChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMDeviceAdapter, QString, bHwAddress, &NMDeviceAdapter::hwAddressChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMDeviceAdapter, NMDeviceState::Enum, bState, &NMDeviceAdapter::stateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMDeviceAdapter, NMDeviceType::Enum, bType, &NMDeviceAdapter::typeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMDeviceAdapter, QList<QDBusObjectPath>, bAvailableConnections, &NMDeviceAdapter::availableConnectionsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMDeviceAdapter, QDBusObjectPath, bActiveConnection, &NMDeviceAdapter::activeConnectionChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMDeviceAdapter, deviceProperties);
	QS_DBUS_PROPERTY_BINDING(NMDeviceAdapter, pName, bInterface, deviceProperties, "Interface");
	QS_DBUS_PROPERTY_BINDING(NMDeviceAdapter, pAddress, bHwAddress, deviceProperties, "HwAddress");
	QS_DBUS_PROPERTY_BINDING(NMDeviceAdapter, pType, bType, deviceProperties, "DeviceType");
	QS_DBUS_PROPERTY_BINDING(NMDeviceAdapter, pState, bState, deviceProperties, "State");
	QS_DBUS_PROPERTY_BINDING(NMDeviceAdapter, pAvailableConnections, bAvailableConnections, deviceProperties, "AvailableConnections");
	QS_DBUS_PROPERTY_BINDING(NMDeviceAdapter, pActiveConnection, bActiveConnection, deviceProperties, "ActiveConnection");
	// clang-format on

	DBusNMDeviceProxy* proxy = nullptr;
};

} // namespace qs::network
