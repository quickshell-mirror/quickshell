#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "nm/enums.hpp"

namespace qs::network {

///! Connection state.
class NetworkConnectionState: public QObject {
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
	Q_INVOKABLE static QString toString(NetworkConnectionState::Enum state);
};

///! A Network device.
class NetworkDevice: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("Devices can only be acquired through Network");
	// clang-format off
	/// The name of the device's control interface.
	Q_PROPERTY(QString name READ name NOTIFY nameChanged BINDABLE bindableName);
	/// The hardware address of the device in the XX:XX:XX:XX:XX:XX format.
	Q_PROPERTY(QString address READ address NOTIFY addressChanged BINDABLE bindableAddress);
	/// Connection state of the device.
	Q_PROPERTY(qs::network::NetworkConnectionState::Enum state READ state NOTIFY stateChanged BINDABLE bindableState);
	/// A more specific device state when the backend is NetworkManager.
	Q_PROPERTY(qs::network::NMDeviceState::Enum nmState READ nmState NOTIFY nmStateChanged BINDABLE bindableNmState);
	// clang-format on

signals:
	void nameChanged();
	void addressChanged();
	void stateChanged();
	void nmStateChanged();
	void requestDisconnect();

public:
	explicit NetworkDevice(QObject* parent = nullptr);

	/// Disconnects the device and prevents it from automatically activating further connections.
	Q_INVOKABLE void disconnect();

	[[nodiscard]] QString name() const { return this->bName; };
	[[nodiscard]] QString address() const { return this->bAddress; };
	[[nodiscard]] NetworkConnectionState::Enum state() const { return this->bState; };
	[[nodiscard]] NMDeviceState::Enum nmState() const { return this->bNmState; };
	QBindable<QString> bindableName() { return &this->bName; };
	QBindable<QString> bindableAddress() { return &this->bAddress; };
	QBindable<NetworkConnectionState::Enum> bindableState() { return &this->bState; };
	QBindable<NMDeviceState::Enum> bindableNmState() { return &this->bNmState; };

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, QString, bName, &NetworkDevice::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, QString, bAddress, &NetworkDevice::addressChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, NetworkConnectionState::Enum, bState, &NetworkDevice::stateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, NMDeviceState::Enum, bNmState, &NetworkDevice::nmStateChanged);
	// clang-format on
};

} // namespace qs::network

QDebug operator<<(QDebug debug, const qs::network::NetworkDevice* device);
