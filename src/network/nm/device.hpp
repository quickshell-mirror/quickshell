#pragma once

#include <qdbusextratypes.h>
#include <qhash.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "connection.hpp"
#include "dbus_nm_device.h"
#include "types.hpp"

namespace qs::dbus {

template <>
struct DBusDataTransform<qs::network::NMDeviceState::Enum> {
	using Wire = quint32;
	using Data = qs::network::NMDeviceState::Enum;
	static DBusResult<Data> fromWire(Wire wire);
};

} // namespace qs::dbus

namespace qs::network {

// Proxy of a /org/freedesktop/NetworkManager/Device/* object.
// Only the members from the org.freedesktop.NetworkManager.Device interface.
// Owns the lifetime of NMActiveConnection(s) and NMConnectionSetting(s).
class NMDevice: public QObject {
	Q_OBJECT;

public:
	explicit NMDevice(const QString& path, QObject* parent = nullptr);

	[[nodiscard]] virtual bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] QString interface() const { return this->bInterface; };
	[[nodiscard]] QString hwAddress() const { return this->bHwAddress; };
	[[nodiscard]] bool managed() const { return this->bManaged; };
	[[nodiscard]] NMDeviceState::Enum state() const { return this->bState; };
	[[nodiscard]] bool autoconnect() const { return this->bAutoconnect; };
	[[nodiscard]] NMActiveConnection* activeConnection() const { return this->mActiveConnection; };

signals:
	void activateConnection(const QDBusObjectPath& connPath, const QDBusObjectPath& devPath);
	void addAndActivateConnection(
	    const ConnectionSettingsMap& settings,
	    const QDBusObjectPath& devPath,
	    const QDBusObjectPath& apPath
	);
	void connectionLoaded(NMConnectionSettings* connection);
	void connectionRemoved(NMConnectionSettings* connection);
	void availableConnectionPathsChanged(QList<QDBusObjectPath> paths);
	void activeConnectionPathChanged(const QDBusObjectPath& connection);
	void activeConnectionLoaded(NMActiveConnection* active);
	void interfaceChanged(const QString& interface);
	void hwAddressChanged(const QString& hwAddress);
	void managedChanged(bool managed);
	void stateChanged(NMDeviceState::Enum state);
	void autoconnectChanged(bool autoconnect);

public slots:
	void disconnect();
	void setAutoconnect(bool autoconnect);
	void setManaged(bool managed);

private slots:
	void onAvailableConnectionPathsChanged(const QList<QDBusObjectPath>& paths);
	void onActiveConnectionPathChanged(const QDBusObjectPath& path);

private:
	void registerConnection(const QString& path);

	QHash<QString, NMConnectionSettings*> mConnections;
	QHash<QString, NMConnection*> mFrontendConnections;
	NMActiveConnection* mActiveConnection = nullptr;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMDevice, QString, bInterface, &NMDevice::interfaceChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMDevice, QString, bHwAddress, &NMDevice::hwAddressChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMDevice, bool, bManaged, &NMDevice::managedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMDevice, NMDeviceState::Enum, bState, &NMDevice::stateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMDevice, bool, bAutoconnect, &NMDevice::autoconnectChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMDevice, QList<QDBusObjectPath>, bAvailableConnections, &NMDevice::availableConnectionPathsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMDevice, QDBusObjectPath, bActiveConnection, &NMDevice::activeConnectionPathChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMDeviceAdapter, deviceProperties);
	QS_DBUS_PROPERTY_BINDING(NMDevice, pName, bInterface, deviceProperties, "Interface");
	QS_DBUS_PROPERTY_BINDING(NMDevice, pAddress, bHwAddress, deviceProperties, "HwAddress");
	QS_DBUS_PROPERTY_BINDING(NMDevice, pManaged, bManaged, deviceProperties, "Managed");
	QS_DBUS_PROPERTY_BINDING(NMDevice, pState, bState, deviceProperties, "State");
	QS_DBUS_PROPERTY_BINDING(NMDevice, pAutoconnect, bAutoconnect, deviceProperties, "Autoconnect");
	QS_DBUS_PROPERTY_BINDING(NMDevice, pAvailableConnections, bAvailableConnections, deviceProperties, "AvailableConnections");
	QS_DBUS_PROPERTY_BINDING(NMDevice, pActiveConnection, bActiveConnection, deviceProperties, "ActiveConnection");
	// clang-format on

	DBusNMDeviceProxy* deviceProxy = nullptr;
};

} // namespace qs::network
