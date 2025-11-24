#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/model.hpp"
#include "device.hpp"

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
class Network: public QObject {
	Q_OBJECT;
	QML_SINGLETON;
	QML_NAMED_ELEMENT(Network);
	// clang-format off
	/// A list of all network devices.
	Q_PROPERTY(UntypedObjectModel* devices READ devices CONSTANT);
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::Network::Device>*);
	/// The backend being used to power the Network service.
	Q_PROPERTY(qs::network::NetworkBackendType::Enum backend READ backend CONSTANT);
	/// Switch for the rfkill software block of all wireless devices.
	Q_PROPERTY(bool wifiEnabled READ wifiEnabled WRITE setWifiEnabled NOTIFY wifiEnabledChanged);
	/// Switch for the rfkill hardware block of all wireless devices.
	Q_PROPERTY(bool wifiHardwareEnabled READ wifiHardwareEnabled NOTIFY wifiHardwareEnabledChanged BINDABLE bindableWifiHardwareEnabled);
	// clang-format on

public:
	explicit Network(QObject* parent = nullptr);

	[[nodiscard]] ObjectModel<NetworkDevice>* devices() { return &this->mDevices; };
	[[nodiscard]] NetworkBackendType::Enum backend() const { return this->mBackendType; };
	QBindable<bool> bindableWifiEnabled() { return &this->bWifiEnabled; };
	[[nodiscard]] bool wifiEnabled() const { return this->bWifiEnabled; };
	void setWifiEnabled(bool enabled);
	QBindable<bool> bindableWifiHardwareEnabled() { return &this->bWifiHardwareEnabled; };
	[[nodiscard]] bool wifiHardwareEnabled() const { return this->bWifiHardwareEnabled; };

signals:
	void requestSetWifiEnabled(bool enabled);
	void wifiEnabledChanged();
	void wifiHardwareEnabledChanged();

public slots:
	void onDeviceAdded(NetworkDevice* dev);
	void onDeviceRemoved(NetworkDevice* dev);

private:
	ObjectModel<NetworkDevice> mDevices {this};
	NetworkBackend* mBackend = nullptr;
	NetworkBackendType::Enum mBackendType = NetworkBackendType::None;
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(Network, bool, bWifiEnabled, &Network::wifiEnabledChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Network, bool, bWifiHardwareEnabled, &Network::wifiHardwareEnabledChanged);
	// clang-format on
};

///! A base network.
class BaseNetwork: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("BaseNetwork can only be aqcuired through network devices");

	/// The name of the network.
	Q_PROPERTY(QString name READ name CONSTANT);
	/// True if the network is connected.
	Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged BINDABLE bindableConnected);

public:
	explicit BaseNetwork(QString name, QObject* parent = nullptr);

	[[nodiscard]] QString name() const { return this->mName; };
	QBindable<bool> bindableConnected() { return &this->bConnected; }
	[[nodiscard]] bool connected() const { return this->bConnected; };

signals:
	void connectedChanged();

protected:
	QString mName;
	Q_OBJECT_BINDABLE_PROPERTY(BaseNetwork, bool, bConnected, &BaseNetwork::connectedChanged);
};

} // namespace qs::network
