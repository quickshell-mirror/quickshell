#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/model.hpp"
#include "device.hpp"

namespace qs::network {

///! The connection state of a Network.
class NetworkState: public QObject {
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
	Q_INVOKABLE static QString toString(NetworkState::Enum state);
};

///! The backend supplying the Network service.
class NetworkBackendType: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		None = 0,
		NetworkManager = 1,
		FreeBSD = 2,
	};
	Q_ENUM(Enum);
};

class NetworkBackend: public QObject {
	Q_OBJECT;

public:
	[[nodiscard]] virtual bool isAvailable() const = 0;

protected:
	explicit NetworkBackend(QObject* parent = nullptr): QObject(parent) {};
};

///! The Network service.
/// An interface to a network backend (NetworkManager or FreeBSD),
/// which can be used to view, configure, and connect to various networks.
class Networking: public QObject {
	Q_OBJECT;
	QML_SINGLETON;
	QML_ELEMENT;
	// clang-format off
	/// A list of all network devices.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::network::NetworkDevice>*);
	Q_PROPERTY(UntypedObjectModel* devices READ devices CONSTANT);
	/// The backend being used to power the Network service.
	Q_PROPERTY(qs::network::NetworkBackendType::Enum backend READ backend CONSTANT);
	/// Switch for the rfkill software block of all wireless devices.
	Q_PROPERTY(bool wifiEnabled READ wifiEnabled WRITE setWifiEnabled NOTIFY wifiEnabledChanged);
	/// State of the rfkill hardware block of all wireless devices.
	Q_PROPERTY(bool wifiHardwareEnabled READ default NOTIFY wifiHardwareEnabledChanged BINDABLE bindableWifiHardwareEnabled);
	// clang-format on

public:
	explicit Networking(QObject* parent = nullptr);

	[[nodiscard]] ObjectModel<NetworkDevice>* devices() { return &this->mDevices; };
	[[nodiscard]] NetworkBackendType::Enum backend() const { return this->mBackendType; };
	QBindable<bool> bindableWifiEnabled() { return &this->bWifiEnabled; };
	[[nodiscard]] bool wifiEnabled() const { return this->bWifiEnabled; };
	void setWifiEnabled(bool enabled);
	QBindable<bool> bindableWifiHardwareEnabled() { return &this->bWifiHardwareEnabled; };

signals:
	void requestSetWifiEnabled(bool enabled);
	void wifiEnabledChanged();
	void wifiHardwareEnabledChanged();

private slots:
	void deviceAdded(NetworkDevice* dev);
	void deviceRemoved(NetworkDevice* dev);

private:
	ObjectModel<NetworkDevice> mDevices {this};
	NetworkBackend* mBackend = nullptr;
	NetworkBackendType::Enum mBackendType = NetworkBackendType::None;
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(Networking, bool, bWifiEnabled, &Networking::wifiEnabledChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Networking, bool, bWifiHardwareEnabled, &Networking::wifiHardwareEnabledChanged);
	// clang-format on
};

///! A network.
class Network: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("BaseNetwork can only be aqcuired through network devices");

	// clang-format off
	/// The name of the network.
	Q_PROPERTY(QString name READ name CONSTANT);
	/// True if the network is connected.
	Q_PROPERTY(bool connected READ default NOTIFY connectedChanged BINDABLE bindableConnected);
	/// The connectivity state of the network.
	Q_PROPERTY(NetworkState::Enum state READ default NOTIFY stateChanged BINDABLE bindableState);
	/// If the network is currently connecting or disconnecting. Shorthand for checking @@state.
	Q_PROPERTY(bool stateChanging READ default NOTIFY stateChangingChanged BINDABLE bindableStateChanging);
	// clang-format on

public:
	explicit Network(QString name, QObject* parent = nullptr);

	[[nodiscard]] QString name() const { return this->mName; };
	QBindable<bool> bindableConnected() { return &this->bConnected; }
	QBindable<NetworkState::Enum> bindableState() { return &this->bState; }
	QBindable<bool> bindableStateChanging() { return &this->bStateChanging; }

signals:
	void connectedChanged();
	void stateChanged();
	void stateChangingChanged();

protected:
	QString mName;

	Q_OBJECT_BINDABLE_PROPERTY(Network, bool, bConnected, &Network::connectedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Network, NetworkState::Enum, bState, &Network::stateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Network, bool, bStateChanging, &Network::stateChangingChanged);
};

} // namespace qs::network
