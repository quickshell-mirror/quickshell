#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

namespace qs::network {

///! Connection state of a NetworkDevice.
class DeviceConnectionState: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		Unknown = 0,
		Connecting = 1,
		Connected = 2,
		Disconnecting = 3,
		Disconnected = 4,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(DeviceConnectionState::Enum state);
};

///! Type of network device.
class DeviceType: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		None = 0,
		Wifi = 1,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(DeviceType::Enum type);
};

///! NetworkManager-specific device state.
/// In sync with https://networkmanager.dev/docs/api/latest/nm-dbus-types.html#NMDeviceState.
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

///! A network device.
/// When @@type is `Wifi`, the device is a @@WifiDevice, which can be used to scan for and connect to access points.
class NetworkDevice: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("Devices can only be acquired through Network");
	// clang-format off
	/// The device type.
	Q_PROPERTY(DeviceType::Enum type READ type CONSTANT);
	/// The name of the device's control interface.
	Q_PROPERTY(QString name READ name NOTIFY nameChanged BINDABLE bindableName);
	/// The hardware address of the device in the XX:XX:XX:XX:XX:XX format.
	Q_PROPERTY(QString address READ default NOTIFY addressChanged BINDABLE bindableAddress);
	/// True if the device is connected.
	Q_PROPERTY(bool connected READ default NOTIFY connectedChanged BINDABLE bindableConnected);
	/// Connection state of the device.
	Q_PROPERTY(qs::network::DeviceConnectionState::Enum state READ default NOTIFY stateChanged BINDABLE bindableState);
	/// A more specific device state.
	///
	/// > [!WARNING] Only valid for the NetworkManager backend. 
	Q_PROPERTY(qs::network::NMDeviceState::Enum nmState READ default NOTIFY nmStateChanged BINDABLE bindableNmState);
	/// True if the device is managed by NetworkManager.
	///
	/// > [!WARNING] Only valid for the NetworkManager backend.
	Q_PROPERTY(bool nmManaged READ nmManaged WRITE setNmManaged NOTIFY nmManagedChanged)
	/// True if the device is allowed to autoconnect to a network.
	Q_PROPERTY(bool autoconnect READ autoconnect WRITE setAutoconnect NOTIFY autoconnectChanged);
	// clang-format on

public:
	explicit NetworkDevice(DeviceType::Enum type, QObject* parent = nullptr);

	/// Disconnects the device and prevents it from automatically activating further connections.
	Q_INVOKABLE void disconnect();

	[[nodiscard]] DeviceType::Enum type() const { return this->mType; };
	QBindable<QString> bindableName() { return &this->bName; };
	[[nodiscard]] QString name() const { return this->bName; };
	QBindable<QString> bindableAddress() { return &this->bAddress; };
	QBindable<bool> bindableConnected() { return &this->bConnected; };
	QBindable<DeviceConnectionState::Enum> bindableState() { return &this->bState; };
	QBindable<NMDeviceState::Enum> bindableNmState() { return &this->bNmState; };
	QBindable<bool> bindableNmManaged() { return &this->bNmManaged; };
	[[nodiscard]] bool nmManaged() { return this->bNmManaged; };
	void setNmManaged(bool managed);
	QBindable<bool> bindableAutoconnect() { return &this->bAutoconnect; };
	[[nodiscard]] bool autoconnect() { return this->bAutoconnect; };
	void setAutoconnect(bool autoconnect);

signals:
	void requestDisconnect();
	void requestSetAutoconnect(bool autoconnect);
	void requestSetNmManaged(bool managed);
	void nameChanged();
	void addressChanged();
	void connectedChanged();
	void stateChanged();
	void nmStateChanged();
	void nmManagedChanged();
	void autoconnectChanged();

private:
	DeviceType::Enum mType;
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, QString, bName, &NetworkDevice::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, QString, bAddress, &NetworkDevice::addressChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, bool, bConnected, &NetworkDevice::connectedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, DeviceConnectionState::Enum, bState, &NetworkDevice::stateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, NMDeviceState::Enum, bNmState, &NetworkDevice::nmStateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, bool, bNmManaged, &NetworkDevice::nmManagedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, bool, bAutoconnect, &NetworkDevice::autoconnectChanged);
	// clang-format on
};

} // namespace qs::network
