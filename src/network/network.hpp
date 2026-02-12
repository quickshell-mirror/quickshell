#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/model.hpp"
#include "device.hpp"
#include "enums.hpp"
#include "nm_settings.hpp"

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
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::network::NMSettings>*);
	Q_PROPERTY(UntypedObjectModel* nmSettings READ nmSettings CONSTANT);
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
	/// Attempt to connect to the network with a specific @@nmSettings.
	///
	/// > [!WARNING] Only valid for the NetworkManager backend.
	Q_INVOKABLE void connectWithSettings(NMSettings* settings);
	/// Disconnect from the network.
	Q_INVOKABLE void disconnect();
	/// Forget all connection settings for this network.
	Q_INVOKABLE void forget();

	void settingsAdded(NMSettings* settings);
	void settingsRemoved(NMSettings* settings);

	// clang-format off
	[[nodiscard]] QString name() const { return this->mName; }
	[[nodiscard]] ObjectModel<NMSettings>* nmSettings() { return &this->mNmSettings; }
	QBindable<bool> bindableConnected() { return &this->bConnected; }
	QBindable<bool> bindableKnown() { return &this->bKnown; }
	[[nodiscard]] NetworkState::Enum state() const { return this->bState; }
	QBindable<NetworkState::Enum> bindableState() { return &this->bState; }
	[[nodiscard]] NMNetworkStateReason::Enum nmStateReason() const { return this->bNmStateReason; }
	QBindable<NMNetworkStateReason::Enum> bindableNmStateReason() { return &this->bNmStateReason; }
	[[nodiscard]] NMDeviceStateReason::Enum nmDeviceFailReason() const { return this->bNmDeviceFailReason; }
	QBindable<NMDeviceStateReason::Enum> bindableNmDeviceFailReason() { return &this->bNmDeviceFailReason; }
	[[nodiscard]] NMSettings* activeSettings() const { return this->bActiveSettings; }
	QBindable<NMSettings*> bindableActiveSettings() { return &this->bActiveSettings; }
	QBindable<bool> bindableStateChanging() { return &this->bStateChanging; }
	// clang-format on

signals:
	void requestConnect();
	void requestConnectWithSettings(NMSettings* settings);
	void requestDisconnect();
	void requestForget();
	void connectedChanged();
	void knownChanged();
	void stateChanged();
	void nmStateReasonChanged();
	void nmDeviceFailReasonChanged();
	void activeSettingsChanged();
	void stateChangingChanged();

protected:
	QString mName;
	ObjectModel<NMSettings> mNmSettings {this};

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(Network, bool, bConnected, &Network::connectedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Network, bool, bKnown, &Network::knownChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Network, NetworkState::Enum, bState, &Network::stateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Network, NMNetworkStateReason::Enum, bNmStateReason, &Network::nmStateReasonChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Network, NMDeviceStateReason::Enum, bNmDeviceFailReason, &Network::nmDeviceFailReasonChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Network, NMSettings*, bActiveSettings, &Network::activeSettingsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Network, bool, bStateChanging, &Network::stateChangingChanged);
	// clang-format on
};

///! NetworkManager connection context.
/// This is a creatable object and it should be provided a network.
/// It emits useful signals about attempts to connect to the network.
class NMConnectionContext: public QObject {
	Q_OBJECT;
	QML_ELEMENT;

	// clang-format off
	/// A network to provide connection context for or `null`.
	Q_PROPERTY(Network* network READ network WRITE setNetwork NOTIFY networkChanged)
	/// The last connection settings that context signals were emitted for or `null`.
	Q_PROPERTY(NMSettings* settings READ default NOTIFY settingsChanged BINDABLE bindableSettings)
	// clang-format on

public:
	explicit NMConnectionContext(QObject* parent = nullptr);

	[[nodiscard]] Network* network() const { return this->bNetwork; };
	void setNetwork(Network* network);
	QBindable<NMSettings*> bindableSettings() { return &this->bSettings; };

signals:
	void networkChanged();
	void settingsChanged();
	/// The connection was successfully activated.
	void activated();
	/// The connection is activating.
	void activating();
	/// Secrets were required, but not provided.
	void noSecrets();
	/// 802.1x supplicant disconnected.
	void supplicantDisconnect();
	/// 802.1x supplicant failed.
	void supplicantFailed();
	/// 802.1x suuplicant took too long to authenticate.
	void supplicantTimeout();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionContext, Network*, bNetwork, &NMConnectionContext::networkChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMConnectionContext, NMSettings*, bSettings, &NMConnectionContext::settingsChanged);
	// clang-format on
};

} // namespace qs::network
