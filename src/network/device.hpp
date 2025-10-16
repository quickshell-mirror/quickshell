#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "nm/enums.hpp"

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

///! A network device.
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
	Q_PROPERTY(QString address READ address NOTIFY addressChanged BINDABLE bindableAddress);
	/// Connection state of the device.
	Q_PROPERTY(qs::network::DeviceConnectionState::Enum state READ state NOTIFY stateChanged BINDABLE bindableState);
	/// A more specific device state when the backend is NetworkManager.
	Q_PROPERTY(qs::network::NMDeviceState::Enum nmState READ nmState NOTIFY nmStateChanged BINDABLE bindableNmState);
	// clang-format on

public:
	explicit NetworkDevice(DeviceType::Enum type, QObject* parent = nullptr);

	/// Disconnects the device and prevents it from automatically activating further connections.
	Q_INVOKABLE void disconnect();

	[[nodiscard]] DeviceType::Enum type() const { return this->mType; };
	QBindable<QString> bindableName() { return &this->bName; };
	[[nodiscard]] QString name() const { return this->bName; };
	QBindable<QString> bindableAddress() { return &this->bAddress; };
	[[nodiscard]] QString address() const { return this->bAddress; };
	QBindable<DeviceConnectionState::Enum> bindableState() { return &this->bState; };
	[[nodiscard]] DeviceConnectionState::Enum state() const { return this->bState; };
	QBindable<NMDeviceState::Enum> bindableNmState() { return &this->bNmState; };
	[[nodiscard]] NMDeviceState::Enum nmState() const { return this->bNmState; };

signals:
	void requestDisconnect();
	void nameChanged();
	void addressChanged();
	void stateChanged();
	void nmStateChanged();

private:
	DeviceType::Enum mType;
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, QString, bName, &NetworkDevice::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, QString, bAddress, &NetworkDevice::addressChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, DeviceConnectionState::Enum, bState, &NetworkDevice::stateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, NMDeviceState::Enum, bNmState, &NetworkDevice::nmStateChanged);
	// clang-format on
};

} // namespace qs::network
