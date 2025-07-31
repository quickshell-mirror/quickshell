#pragma once

#include <qcontainerfwd.h>
#include <qdbusextratypes.h>
#include <qdbusservicewatcher.h>
#include <qhash.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/model.hpp"
#include "nm/enums.hpp"

namespace qs::network {

///! A wifi network represents an available connection or access point on a wireless device.
class WifiNetwork: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("Wifi items can only be acquired through Network");
	// clang-format off
	/// The SSID (service set identifier) of the wifi network
	Q_PROPERTY(QString ssid READ default NOTIFY ssidChanged BINDABLE bindableSsid);
	// The current signal strength of the best access point on the network, in percent.
	Q_PROPERTY(quint8 signalStrength READ default NOTIFY signalStrengthChanged BINDABLE bindableSignalStrength);
	/// True if the wireless device is currently connected to this wifi network.
	Q_PROPERTY(bool connected READ default NOTIFY connectedChanged BINDABLE bindableConnected);
	/// The security type of the wifi network when the backend is NetworkManager. Otherwise NMWirelessSecurityType::Unknown.
	Q_PROPERTY(NMWirelessSecurityType::Enum nmSecurity READ default NOTIFY nmSecurityChanged BINDABLE bindableNmSecurity);
	/// True if the wifi network has a known connection profile saved.
	Q_PROPERTY(bool known READ default NOTIFY knownChanged BINDABLE bindableKnown);
	// clang-format on

signals:
	void ssidChanged();
	void signalStrengthChanged();
	void connectedChanged();
	void nmSecurityChanged();
	void knownChanged();
	void requestConnect();

public slots:
	void setSsid(const QString& ssid);
	void setSignalStrength(quint8 signalStrength);
	void setConnected(bool connected);
	void setNmSecurity(NMWirelessSecurityType::Enum security);
	void setKnown(bool known);

public:
	explicit WifiNetwork(QObject* parent = nullptr);

	[[nodiscard]] QBindable<QString> bindableSsid() const { return &this->bSsid; };
	[[nodiscard]] QBindable<quint8> bindableSignalStrength() const { return &this->bSignalStrength; };
	[[nodiscard]] QBindable<bool> bindableConnected() const { return &this->bConnected; };
	[[nodiscard]] QBindable<NMWirelessSecurityType::Enum> bindableNmSecurity() const {
		return &this->bNmSecurity;
	};
	[[nodiscard]] QBindable<bool> bindableKnown() const { return &this->bKnown; };

	// Q_INVOKABLE void connect();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, QString, bSsid, &WifiNetwork::ssidChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, quint8, bSignalStrength, &WifiNetwork::signalStrengthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, bool, bConnected, &WifiNetwork::connectedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, NMWirelessSecurityType::Enum, bNmSecurity, &WifiNetwork::nmSecurityChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, bool, bKnown, &WifiNetwork::knownChanged);
	// clang-format on
};

///! Type of Network device.
class NetworkDeviceType: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		///! A generic device.
		Other = 0,
		///! An 802.11 Wi-Fi device.
		Wireless = 1,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(NetworkDeviceType::Enum type);
};

///! Connection state of a Network device.
class NetworkDeviceState: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		/// The device state is unknown.
		Unknown = 0,
		/// The device is not connected.
		Disconnected = 1,
		/// The device is connected.
		Connected = 2,
		/// The device is disconnecting.
		Disconnecting = 3,
		/// The device is connecting.
		Connecting = 4,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(NetworkDeviceState::Enum state);
};

///! A tracked Network device.
class NetworkDevice: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("Devices can only be acquired through Network");

	// clang-format off
	/// The name of the device's interface.
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);
	/// The hardware address of the device's interface in the XX:XX:XX:XX:XX:XX format.
	Q_PROPERTY(QString address READ default NOTIFY addressChanged BINDABLE bindableAddress);
	/// Connection state of the device.
	Q_PROPERTY(NetworkDeviceState::Enum state READ default NOTIFY stateChanged BINDABLE bindableState);
	/// Type of device.
	Q_PROPERTY(NetworkDeviceType::Enum type READ type CONSTANT);
	// clang-format on

signals:
	void nameChanged();
	void addressChanged();
	void stateChanged();
	void requestDisconnect();

public slots:
	void setName(const QString& name);
	void setAddress(const QString& address);
	void setState(NetworkDeviceState::Enum state);

public:
	explicit NetworkDevice(QObject* parent = nullptr);

	/// Disconnects the device and prevents it from automatically activating further connections.
	Q_INVOKABLE void disconnect();
	[[nodiscard]] virtual NetworkDeviceType::Enum type() const { return NetworkDeviceType::Other; };

	[[nodiscard]] QBindable<QString> bindableName() const { return &this->bName; };
	[[nodiscard]] QBindable<QString> bindableAddress() const { return &this->bAddress; };
	[[nodiscard]] QBindable<NetworkDeviceState::Enum> bindableState() const { return &this->bState; };

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, QString, bName, &NetworkDevice::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, QString, bAddress, &NetworkDevice::addressChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NetworkDevice, NetworkDeviceState::Enum, bState, &NetworkDevice::stateChanged);
	// clang-format on
};

///! Wireless variant of a tracked network device.
class NetworkWifiDevice: public NetworkDevice {
	Q_OBJECT;

	// clang-format off
	/// True if the wifi device is currently scanning for available wifi networks.
	Q_PROPERTY(bool scanning READ default NOTIFY scanningChanged BINDABLE bindableScanning);
	/// The currently active wifi network
	// Q_PROPERTY(WifiNetwork activeNetwork READ networks CONSTANT);
	/// A list of all wifi networks available to this wifi device
	Q_PROPERTY(UntypedObjectModel* networks READ networks CONSTANT);
	QSDOC_TYPE_OVERRIDE(ObjectModel<WifiNetwork>*)
	//clang-format on

signals:
	void requestScan();
	void scanningChanged();

public slots:
	void scanComplete();
	void wifiNetworkAdded(WifiNetwork* network);
	void wifiNetworkRemoved(WifiNetwork* network);

public:
	explicit NetworkWifiDevice(QObject* parent = nullptr);
	[[nodiscard]] NetworkDeviceType::Enum type() const override { return NetworkDeviceType::Wireless; };

	/// Request the wireless device to scan for available WiFi networks.
	Q_INVOKABLE void scan();

	[[nodiscard]] QBindable<bool> bindableScanning() { return &this->bScanning; };

	UntypedObjectModel* networks() { return &this->mNetworks; };

private:
	ObjectModel<WifiNetwork> mNetworks {this};

	Q_OBJECT_BINDABLE_PROPERTY(NetworkWifiDevice, bool, bScanning, &NetworkWifiDevice::scanningChanged);
};

class NetworkBackend: public QObject {
	Q_OBJECT;

public:
	[[nodiscard]] virtual bool isAvailable() const = 0;

protected:
	explicit NetworkBackend(QObject* parent = nullptr): QObject(parent) {};
};

///! Network manager
/// Provides access to network devices.
class Network: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(Network);
	QML_SINGLETON;

	// clang-format off
	/// The default wifi device. Usually there is only one. This defaults to the first wifi device registered.
	// Q_PROPERTY(WirelessNetworkDevice* defaultWifiDevice READ defaultWifiDevice CONSTANT);
	/// A list of all network devices.
	Q_PROPERTY(UntypedObjectModel* devices READ devices CONSTANT);
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::network::NetworkDevice>*);
	// clang-format on

public slots:
	void addDevice(NetworkDevice* device);
	void removeDevice(NetworkDevice* device);

public:
	explicit Network(QObject* parent = nullptr);
	[[nodiscard]] UntypedObjectModel* devices() { return backend ? &this->mDevices : nullptr; };

private:
	ObjectModel<NetworkDevice> mDevices {this};
	class NetworkBackend* backend = nullptr;
};

} // namespace qs::network
