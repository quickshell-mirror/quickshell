#pragma once
#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "dbus_device.h"

namespace qs::service::networkmanager {

class NetworkManagerDeviceType: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		Ethernet = 1,
		Wifi = 2,
		Unused1 = 3,
		Unused2 = 4,
		Bluetooth = 5,
		OLPCMesh = 6,
		WiMAX = 7,
		Modem = 8,
		InfiniBand = 9,
		Bond = 10,
		VLAN = 11,
		ADSL = 12,
		Bridge = 13,
		Team = 14,
		TUN = 16,
		Tunnel = 17,
		MACVLAN = 18,
		VXLAN = 19,
		VETH = 20,
		MACsec = 21,
		Dummy = 22,
		PPP = 23,
		OVSInterface = 24,
		OVSPort = 25,
		OVSBridge = 26,
		WPAN = 27,
		SixLoWPAN = 28,
		WireGuard = 29,
		WifiP2P = 30,
		VRF = 31,
		Loopback = 32,
		HSR = 33,
		IPVLAN = 34,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(NetworkManagerDeviceType::Enum type);
};

class NetworkManagerDeviceState: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		Unknown = 0,
		Unmanaged = 10,
		Unavailable = 20,
		Disconnected = 30,
		Prepare = 40,
		Config = 50,
		NeedAuth = 60,
		IPConfig = 70,
		IPCheck = 80,
		Secondaries = 90,
		Activated = 100,
		Deactivating = 110,
		Failed = 120,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(NetworkManagerDeviceState::Enum state);
};

} // namespace qs::service::networkmanager

namespace qs::dbus {

template <>
struct DBusDataTransform<qs::service::networkmanager::NetworkManagerDeviceState::Enum> {
	using Wire = quint32;
	using Data = qs::service::networkmanager::NetworkManagerDeviceState::Enum;
	static DBusResult<Data> fromWire(Wire wire);
};

template <>
struct DBusDataTransform<qs::service::networkmanager::NetworkManagerDeviceType::Enum> {
	using Wire = quint32;
	using Data = qs::service::networkmanager::NetworkManagerDeviceType::Enum;
	static DBusResult<Data> fromWire(Wire wire);
};

} // namespace qs::dbus

namespace qs::service::networkmanager {

class NetworkManagerDevice: public QObject {
	Q_OBJECT;
	// clang-format off
	Q_PROPERTY(NetworkManagerDeviceType::Enum type READ default NOTIFY typeChanged BINDABLE	bindableType);
	Q_PROPERTY(NetworkManagerDeviceState::Enum state READ default NOTIFY stateChanged BINDABLE bindableState);
	Q_PROPERTY(QString interface READ default NOTIFY interfaceChanged BINDABLE bindableInterface);
	Q_PROPERTY(bool managed READ default NOTIFY managedChanged BINDABLE bindableManaged);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("NetworkManagerDevices can only be acquired from NetworkManager");

public:
	explicit NetworkManagerDevice(QObject* parent = nullptr);

	void init(const QString& path);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;

	[[nodiscard]] QBindable<NetworkManagerDeviceType::Enum> bindableType() const {
		return &this->bType;
	};
	[[nodiscard]] QBindable<NetworkManagerDeviceState::Enum> bindableState() const {
		return &this->bState;
	};
	[[nodiscard]] QBindable<bool> bindableManaged() const { return &this->bManaged; };
	[[nodiscard]] QBindable<QString> bindableInterface() const { return &this->bInterface; };

signals:
	void typeChanged();
	void stateChanged();
	void interfaceChanged();
	void managedChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NetworkManagerDevice, NetworkManagerDeviceType::Enum, bType, &NetworkManagerDevice::typeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkManagerDevice, NetworkManagerDeviceState::Enum, bState, &NetworkManagerDevice::stateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkManagerDevice, bool, bManaged, &NetworkManagerDevice::managedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkManagerDevice, QString, bInterface, &NetworkManagerDevice::interfaceChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NetworkManagerDevice, deviceProperties);
	QS_DBUS_PROPERTY_BINDING(NetworkManagerDevice, pType, bType, deviceProperties, "DeviceType");
	QS_DBUS_PROPERTY_BINDING(NetworkManagerDevice, pState, bState, deviceProperties, "State");
	QS_DBUS_PROPERTY_BINDING(NetworkManagerDevice, pManaged, bManaged, deviceProperties, "Managed");
	QS_DBUS_PROPERTY_BINDING(NetworkManagerDevice, pInterface, bInterface, deviceProperties, "Interface");
	// clang-format on

	DBusNetworkManagerDevice* device = nullptr;
};

} // namespace qs::service::networkmanager
