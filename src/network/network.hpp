#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/doc.hpp"
#include "enums.hpp"
#include "known_network.hpp"

namespace qs::network {
class NetworkDevice;
}

namespace qs::network {

///! A network.
/// A network. Networks derived from a @@WifiDevice are @@WifiNetwork instances.
class Network: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("Network can only be aqcuired through networking devices");

	// clang-format off
	/// The name of the network.
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);
	/// The device this network belongs to.
	Q_PROPERTY(NetworkDevice* device READ device CONSTANT);
	/// A list of known profiles for this network.
	Q_PROPERTY(QList<KnownNetwork*> knownNetworks READ default NOTIFY knownNetworksChanged BINDABLE bindableKnownNetworks);
	/// True if the network is connected.
	Q_PROPERTY(bool connected READ default NOTIFY connectedChanged BINDABLE bindableConnected);
	/// True if the wifi network has known connection settings saved.
	Q_PROPERTY(bool known READ default NOTIFY knownChanged BINDABLE bindableKnown);
	/// The connectivity state of the network.
	Q_PROPERTY(ConnectionState::Enum state READ default NOTIFY stateChanged BINDABLE bindableState);
	/// If the network is currently connecting or disconnecting. Shorthand for checking @@state.
	Q_PROPERTY(bool stateChanging READ default NOTIFY stateChangingChanged BINDABLE bindableStateChanging);
	// clang-format on

public:
	explicit Network(QString name, NetworkDevice* device, QObject* parent = nullptr);
	/// Attempt to connect to the network.
	///
	/// > [!NOTE] If the network is a @@WifiNetwork and requires secrets, a @@connectionFailed(s)
	/// > signal will be emitted with `NoSecrets`.
	/// > @@WifiNetwork.connectWithPsk() can be used to provide secrets.
	Q_INVOKABLE void connect();
	/// Attempt to connect to the network with the settings of a specific @@knownNetworks entry.
	Q_INVOKABLE void connectToKnownNetwork(KnownNetwork* knownNet);
	/// Disconnect from the network.
	Q_INVOKABLE void disconnect();
	/// Forget all connection settings for this network.
	Q_INVOKABLE void forget();

	void knownNetworkAdded(KnownNetwork* knownNet);
	void knownNetworkRemoved(KnownNetwork* knownNet);

	// clang-format off
	[[nodiscard]] QString name() const { return this->bName; }
	[[nodiscard]] QBindable<QString> bindableName() { return &this->bName; }
	[[nodiscard]] NetworkDevice* device() const { return this->mDevice; }
	QBindable<QList<KnownNetwork*>> bindableKnownNetworks() const { return &this->bKnownNetworks; }
	[[nodiscard]] const QList<KnownNetwork*>& knownNetworks() const { return this->bKnownNetworks; }
	QBindable<bool> bindableConnected() { return &this->bConnected; }
	QBindable<bool> bindableKnown() { return &this->bKnown; }
	[[nodiscard]] ConnectionState::Enum state() const { return this->bState; }
	QBindable<ConnectionState::Enum> bindableState() { return &this->bState; }
	QBindable<bool> bindableStateChanging() { return &this->bStateChanging; }
	// clang-format on

signals:
	/// Signals that a connection to the network has failed because of the given @@ConnectionFailReason.
	void connectionFailed(ConnectionFailReason::Enum reason);

	void nameChanged();
	void connectedChanged();
	void knownChanged();
	void stateChanged();
	void stateChangingChanged();
	void knownNetworksChanged();
	QSDOC_HIDE void requestConnect();
	QSDOC_HIDE void requestConnectWithKnownNetwork(KnownNetwork* settings);
	QSDOC_HIDE void requestDisconnect();
	QSDOC_HIDE void requestForget();

protected:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(Network, QString, bName, &Network::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Network, bool, bConnected, &Network::connectedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Network, bool, bKnown, &Network::knownChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Network, ConnectionState::Enum, bState, &Network::stateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Network, bool, bStateChanging, &Network::stateChangingChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Network, QList<KnownNetwork*>, bKnownNetworks, &Network::knownNetworksChanged);
	// clang-format on

private:
	NetworkDevice* mDevice;
};

} // namespace qs::network
