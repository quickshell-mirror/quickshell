#pragma once
#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "dbus_device.h"

namespace qs::service::networkmanager {

class NMDeviceType: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		Unknown = 0,
		Generic = 14,
		Ethernet = 1,
		Wifi = 2,
		Unused1 = 3,
		Unused2 = 4,
		Bluetooth = 5,
		OlpcMesh = 6,
		Wimax = 7,
		Modem = 8,
		Infiniband = 9,
		Bond = 10,
		Vlan = 11,
		Adsl = 12,
		Bridge = 13,
		Team = 15,
		Tun = 16,
		Tunnel = 17,
		Macvlan = 18,
		Vxlan = 19,
		Veth = 20,
		Macsec = 21,
		Dummy = 22,
		Ppp = 23,
		OvsInterface = 24,
		OvsPort = 25,
		OvsBridge = 26,
		Wpan = 27,
		Lowpan = 28,
		WireGuard = 29,
		WifiP2P = 30,
		Vrf = 31,
		Loopback = 32,
		Hsr = 33,
		Ipvlan = 34,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(NMDeviceType::Enum type);
};

class NMDeviceState: public QObject {
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
	Q_INVOKABLE static QString toString(NMDeviceState::Enum state);
};

} // namespace qs::service::networkmanager

namespace qs::dbus {

template <>
struct DBusDataTransform<qs::service::networkmanager::NMDeviceState::Enum> {
	using Wire = quint32;
	using Data = qs::service::networkmanager::NMDeviceState::Enum;
	static DBusResult<Data> fromWire(Wire wire);
};

template <>
struct DBusDataTransform<qs::service::networkmanager::NMDeviceType::Enum> {
	using Wire = quint32;
	using Data = qs::service::networkmanager::NMDeviceType::Enum;
	static DBusResult<Data> fromWire(Wire wire);
};

} // namespace qs::dbus

namespace qs::service::networkmanager {

class NMDevice: public QObject {
	Q_OBJECT;
	// clang-format off
	Q_PROPERTY(NMDeviceType::Enum type READ default NOTIFY typeChanged BINDABLE	bindableType);
	Q_PROPERTY(NMDeviceState::Enum state READ default NOTIFY stateChanged BINDABLE bindableState);
	Q_PROPERTY(QString interface READ default NOTIFY interfaceChanged BINDABLE bindableInterface);
	Q_PROPERTY(bool managed READ default NOTIFY managedChanged BINDABLE bindableManaged);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("NMDevices can only be acquired from NetworkManager");

public:
	explicit NMDevice(QObject* parent = nullptr);

	void init(const QString& path);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;

	[[nodiscard]] QBindable<NMDeviceType::Enum> bindableType() const {
		return &this->bType;
	};
	[[nodiscard]] QBindable<NMDeviceState::Enum> bindableState() const {
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
	Q_OBJECT_BINDABLE_PROPERTY(NMDevice, NMDeviceType::Enum, bType, &NMDevice::typeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMDevice, NMDeviceState::Enum, bState, &NMDevice::stateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMDevice, bool, bManaged, &NMDevice::managedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMDevice, QString, bInterface, &NMDevice::interfaceChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMDevice, deviceProperties);
	QS_DBUS_PROPERTY_BINDING(NMDevice, pType, bType, deviceProperties, "DeviceType");
	QS_DBUS_PROPERTY_BINDING(NMDevice, pState, bState, deviceProperties, "State");
	QS_DBUS_PROPERTY_BINDING(NMDevice, pManaged, bManaged, deviceProperties, "Managed");
	QS_DBUS_PROPERTY_BINDING(NMDevice, pInterface, bInterface, deviceProperties, "Interface");
	// clang-format on

	DBusNMDevice* device = nullptr;
};

} // namespace qs::service::networkmanager
