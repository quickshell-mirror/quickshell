#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/model.hpp"
#include "device.hpp"
#include "nm/enums.hpp"

namespace qs::network {

///! An available wifi network.
class WifiNetwork: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("WifiNetwork can only be acquired through WifiDevice");
	// clang-format off
	/// The SSID (service set identifier) of the wifi network.
	Q_PROPERTY(QString ssid READ ssid CONSTANT);
	/// The current signal strength of the network, in percent.
	Q_PROPERTY(quint8 signalStrength READ signalStrength NOTIFY signalStrengthChanged BINDABLE bindableSignalStrength);
	/// True if the network is connected.
	Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged BINDABLE bindableConnected);
	/// True if the wifi network has known connection settings saved.
	Q_PROPERTY(bool known READ known NOTIFY knownChanged BINDABLE bindableKnown);
	/// A specific reason for the connection state when the backend is NetworkManager.
	Q_PROPERTY(NMConnectionStateReason::Enum nmReason READ nmReason NOTIFY nmReasonChanged BINDABLE bindableNmReason);
	/// The security type of the wifi network when the backend is NetworkManager.
	Q_PROPERTY(NMWirelessSecurityType::Enum nmSecurity READ nmSecurity NOTIFY nmSecurityChanged BINDABLE bindableNmSecurity);
	// clang-format on

signals:
	void signalStrengthChanged();
	void connectedChanged();
	void knownChanged();
	void nmReasonChanged();
	void nmSecurityChanged();
	void requestConnect();

public:
	explicit WifiNetwork(QString ssid, QObject* parent = nullptr);

	/// Attempt to connect to the wifi network.
	Q_INVOKABLE void connect();
	
	[[nodiscard]] QString ssid() const { return this->mSsid; };
	[[nodiscard]] quint8 signalStrength() const { return this->bSignalStrength; };
	[[nodiscard]] bool connected() const { return this->bConnected; };
	[[nodiscard]] bool known() const { return this->bKnown; };
	[[nodiscard]] NMConnectionStateReason::Enum nmReason() const { return this->bNmReason; };
	[[nodiscard]] NMWirelessSecurityType::Enum nmSecurity() const { return this->bNmSecurity; };
	QBindable<quint8> bindableSignalStrength() { return &this->bSignalStrength; }
	QBindable<bool> bindableConnected() { return &this->bConnected; }
	QBindable<bool> bindableKnown() { return &this->bKnown; }
	QBindable<NMConnectionStateReason::Enum> bindableNmReason() { return &this->bNmReason; }
	QBindable<NMWirelessSecurityType::Enum> bindableNmSecurity() { return &this->bNmSecurity; }

private:
	QString mSsid;
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, quint8, bSignalStrength, &WifiNetwork::signalStrengthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, bool, bConnected, &WifiNetwork::connectedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, bool, bKnown, &WifiNetwork::knownChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, NMConnectionStateReason::Enum, bNmReason, &WifiNetwork::nmReasonChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, NMWirelessSecurityType::Enum, bNmSecurity, &WifiNetwork::nmSecurityChanged);
	// clang-format on
};

///! Wireless variant of a network device.
class WifiDevice: public NetworkDevice {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("WifiDevices can only be acquired through Wifi");

	/// True if the wifi device is currently scanning for available wifi networks.
	Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged);
	/// A list of all wifi networks currently available.
	Q_PROPERTY(UntypedObjectModel* networks READ networks CONSTANT);
	QSDOC_TYPE_OVERRIDE(ObjectModel<WifiNetwork>*)

signals:
	void requestScan();
	void scanningChanged();

public slots:
	void networkAdded(WifiNetwork* net);
	void networkRemoved(WifiNetwork* net);

public:
	explicit WifiDevice(QObject* parent = nullptr);

	/// Request the wireless device to scan for available WiFi networks.
	/// This should be invoked everytime you want to show the user an accurate list of available networks.
	Q_INVOKABLE void scan();

	[[nodiscard]] ObjectModel<WifiNetwork>* networks() { return &this->mNetworks; };
	[[nodiscard]] bool scanning() const { return this->bScanning; };
	QBindable<bool> bindableScanning() { return &this->bScanning; };

private:
	ObjectModel<WifiNetwork> mNetworks {this};
	Q_OBJECT_BINDABLE_PROPERTY(WifiDevice, bool, bScanning, &WifiDevice::scanningChanged);
};

///! A manager for all wifi state and devices.
class Wifi: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("Wifi can only be acquired through Network");
	// clang-format off
	/// A list of all wifi devices.
	Q_PROPERTY(UntypedObjectModel* devices READ devices CONSTANT);
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::network::WifiDevice>*);
	/// True when the wifi software switch is enabled.
	Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged);
	/// True when the wifi hardware switch is enabled.
	Q_PROPERTY(bool hardwareEnabled READ hardwareEnabled NOTIFY hardwareEnabledChanged BINDABLE bindableHardwareEnabled);
	// clang-format on

signals:
	void enabledChanged();
	void hardwareEnabledChanged();
	void defaultDeviceChanged();
	void requestSetEnabled(bool enabled);

public slots:
	void onDeviceAdded(WifiDevice* dev);
	void onDeviceRemoved(WifiDevice* dev);

public:
	explicit Wifi(QObject* parent = nullptr);

	[[nodiscard]] ObjectModel<WifiDevice>* devices() { return &this->mDevices; };
	[[nodiscard]] bool enabled() const { return this->bEnabled; };
	void setEnabled(bool enabled);
	[[nodiscard]] bool hardwareEnabled() const { return this->bHardwareEnabled; };
	QBindable<bool> bindableEnabled() { return &this->bEnabled; };
	QBindable<bool> bindableHardwareEnabled() { return &this->bHardwareEnabled; };

private:
	ObjectModel<WifiDevice> mDevices {this};
	Q_OBJECT_BINDABLE_PROPERTY(Wifi, bool, bEnabled, &Wifi::enabledChanged);
	Q_OBJECT_BINDABLE_PROPERTY(Wifi, bool, bHardwareEnabled, &Wifi::hardwareEnabledChanged);
};

}; // namespace qs::network

QDebug operator<<(QDebug debug, const qs::network::WifiDevice* device);
QDebug operator<<(QDebug debug, const qs::network::WifiNetwork* network);
