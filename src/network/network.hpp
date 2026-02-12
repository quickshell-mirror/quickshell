#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/model.hpp"
#include "device.hpp"
#include "enums.hpp"
#include "nm_connection.hpp"

namespace qs::network {

///! The backend supplying the Network service.
class NetworkBackendType: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		None = 0,
		NetworkManager = 1,
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
/// An interface to a network backend (currently only NetworkManager),
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
	QML_UNCREATABLE("Network can only be aqcuired through networking devices");

	// clang-format off
	/// The name of the network.
	Q_PROPERTY(QString name READ name CONSTANT);
	/// A list of connnection settings profiles for this network.
	///
	/// > [!WARNING] Only valid for the NetworkManager backend. 
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::network::NMConnection>*);
	Q_PROPERTY(UntypedObjectModel* nmConnections READ nmConnections CONSTANT);
	/// The default connection settings profile for this network. This is the connection settings used when connect() is invoked.
	/// Only available when the connection is known.
	///
	/// > [!NOTE] The default connection can only be set while the Network is disconnected.
	/// > [!WARNING] Only valid for the NetworkManager backend.
	Q_PROPERTY(NMConnection* nmDefaultConnection READ nmDefaultConnection WRITE setNmDefaultConnection NOTIFY nmDefaultConnectionChanged BINDABLE bindableNmDefaultConnection);
	/// True if the network is connected.
	Q_PROPERTY(bool connected READ default NOTIFY connectedChanged BINDABLE bindableConnected);
	/// True if the wifi network has known connection settings saved.
	Q_PROPERTY(bool known READ default NOTIFY knownChanged BINDABLE bindableKnown);
	/// The connectivity state of the network.
	Q_PROPERTY(NetworkState::Enum state READ default NOTIFY stateChanged BINDABLE bindableState);
	/// A reason for @@state of the network.
	///
	/// > [!WARNING] Only valid for the NetworkManager backend.
	Q_PROPERTY(NMNetworkStateReason::Enum nmStateReason READ default NOTIFY nmStateReasonChanged BINDABLE bindableNmStateReason);
	/// If the network is currently connecting or disconnecting. Shorthand for checking @@state.
	Q_PROPERTY(bool stateChanging READ default NOTIFY stateChangingChanged BINDABLE bindableStateChanging);
	// clang-format on

public:
	explicit Network(QString name, QObject* parent = nullptr);

	/// Attempt to connect to the network.
	Q_INVOKABLE void connect();
	/// Disconnect from the network.
	Q_INVOKABLE void disconnect();
	/// Forget all connection settings for this network.
	Q_INVOKABLE void forget();

	void connectionAdded(NMConnection* conn);
	void connectionRemoved(NMConnection* conn);

	[[nodiscard]] QString name() const { return this->mName; }
	[[nodiscard]] ObjectModel<NMConnection>* nmConnections() { return &this->mNmConnections; }
	[[nodiscard]] NMConnection* nmDefaultConnection() { return this->bNmDefaultConnection; }
	QBindable<NMConnection*> bindableNmDefaultConnection() { return &this->bNmDefaultConnection; }
	void setNmDefaultConnection(NMConnection* conn);
	QBindable<bool> bindableConnected() { return &this->bConnected; }
	QBindable<bool> bindableKnown() { return &this->bKnown; }
	[[nodiscard]] NetworkState::Enum state() const { return this->bState; }
	QBindable<NetworkState::Enum> bindableState() { return &this->bState; }
	[[nodiscard]] NMNetworkStateReason::Enum nmStateReason() const { return this->bNmStateReason; }
	QBindable<NMNetworkStateReason::Enum> bindableNmStateReason() { return &this->bNmStateReason; }
	QBindable<bool> bindableStateChanging() { return &this->bStateChanging; }

signals:
	void requestSetNmDefaultConnection(NMConnection* conn);
	void requestConnect();
	void requestDisconnect();
	void requestForget();
	void nmDefaultConnectionChanged();
	void connectedChanged();
	void knownChanged();
	void stateChanged();
	void nmStateReasonChanged();
	void stateChangingChanged();

protected:
	QString mName;
	ObjectModel<NMConnection> mNmConnections {this};

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(Network, NMConnection*, bNmDefaultConnection, &Network::nmDefaultConnectionChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Network, bool, bConnected, &Network::connectedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Network, bool, bKnown, &Network::knownChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Network, NetworkState::Enum, bState, &Network::stateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Network, NMNetworkStateReason::Enum, bNmStateReason, &Network::nmStateReasonChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Network, bool, bStateChanging, &Network::stateChangingChanged);
	// clang-format on
};

///! NetworkManager connection context.
/// This is a creatable object and it should be provided a network.
/// It emits helpful signals about the current attempt to connect
/// using the network's @@Quickshell.Networking.Network.nmDefaultConnection.
class NMConnectionContext: public QObject {
	Q_OBJECT;
	QML_ELEMENT;

	// clang-format off
	Q_PROPERTY(Network* network READ network WRITE setNetwork NOTIFY networkChanged)
	// clang-format on

public:
	explicit NMConnectionContext(QObject* parent = nullptr);

	[[nodiscard]] Network* network() const { return this->bNetwork; };
	void setNetwork(Network* network);

signals:
	void networkChanged();
	void connectionChanged();
	/// Authentication to the server failed.
	void loginFailed();
	/// Necessary secrets for the connection were not provided.
	void noSecrets();
	/// The connection attempt was successful.
	void success();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionContext, Network*, bNetwork, &NMConnectionContext::networkChanged);
	// clang-format on
};

} // namespace qs::network
