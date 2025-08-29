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
#include "device.hpp"
#include "nm/enums.hpp"

namespace qs::network {

///! An available wifi network.
class WifiNetwork: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("WifiNetwork can only be acquired through WifiDevice");
	/// The SSID (service set identifier) of the wifi network.
	Q_PROPERTY(QString ssid READ ssid CONSTANT);
	/// The current signal strength of the network, in percent.
	Q_PROPERTY(quint8 signalStrength READ signalStrength NOTIFY signalStrengthChanged);
	/// The connection state of the wifi network.
	Q_PROPERTY(NetworkConnectionState::Enum state READ state NOTIFY stateChanged);
	/// True if the wifi network has known connection settings saved.
	Q_PROPERTY(bool known READ known NOTIFY knownChanged);
	/// The specific reason for the connection state when the backend is NetworkManager.
	Q_PROPERTY(NMConnectionStateReason::Enum nmReason READ nmReason NOTIFY nmReasonChanged);
	/// The security type of the wifi network when the backend is NetworkManager.
	Q_PROPERTY(NMWirelessSecurityType::Enum nmSecurity READ nmSecurity NOTIFY nmSecurityChanged);

signals:
	void signalStrengthChanged();
	void stateChanged();
	void knownChanged();
	void nmReasonChanged();
	void nmSecurityChanged();
	void requestConnect();

public slots:
	void setSignalStrength(quint8 signalStrength);
	void setState(NetworkConnectionState::Enum state);
	void setNmReason(NMConnectionStateReason::Enum reason);
	void setNmSecurity(NMWirelessSecurityType::Enum security);
	void setKnown(bool known);

public:
	explicit WifiNetwork(QString ssid, QObject* parent = nullptr);

	/// Attempt to connect to the wifi network.
	Q_INVOKABLE void connect();

	[[nodiscard]] QString ssid() const { return this->mSsid; };
	[[nodiscard]] quint8 signalStrength() const { return this->mSignalStrength; };
	[[nodiscard]] NetworkConnectionState::Enum state() const { return this->mState; };
	[[nodiscard]] NMConnectionStateReason::Enum nmReason() const { return this->mNmReason; };
	[[nodiscard]] NMWirelessSecurityType::Enum nmSecurity() const { return this->mNmSecurity; };
	[[nodiscard]] bool known() const { return this->mKnown; };

private:
	QString mSsid;
	quint8 mSignalStrength = 0;
	bool mKnown = false;
	NetworkConnectionState::Enum mState = NetworkConnectionState::Disconnected;
	NMConnectionStateReason::Enum mNmReason = NMConnectionStateReason::Unknown;
	NMWirelessSecurityType::Enum mNmSecurity = NMWirelessSecurityType::Unknown;
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
	void scanComplete();
	void networkAdded(WifiNetwork* net);
	void networkRemoved(WifiNetwork* net);

public:
	explicit WifiDevice(QObject* parent = nullptr);

	/// Request the wireless device to scan for available WiFi networks.
	/// This should be invoked everytime you want to show the user an accurate list of available networks.
	Q_INVOKABLE void scan();

	[[nodiscard]] bool scanning() const { return this->mScanning; };
	UntypedObjectModel* networks() { return &this->mNetworks; };

private:
	ObjectModel<WifiNetwork> mNetworks {this};
	bool mScanning = false;
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
	/// The default wifi device.
	/// Useful for choosing the device you'd like to use for any wifi connection.
	/// By default this will be the first device discovered.
	Q_PROPERTY(WifiDevice* defaultDevice READ defaultDevice WRITE setDefaultDevice NOTIFY defaultDeviceChanged);
	/// True when the wifi software switch is enabled.
	Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged);
	/// True when the wifi hardware switch is enabled.
	Q_PROPERTY(bool hardwareEnabled READ hardwareEnabled NOTIFY hardwareEnabledChanged);

signals:
	void enabledChanged();
	void hardwareEnabledChanged();
	void defaultDeviceChanged();
	void requestSetEnabled(bool enabled);

public slots:
	void onDeviceAdded(WifiDevice* dev);
	void onDeviceRemoved(WifiDevice* dev);
	void onEnabledSet(bool enabled);
	void onHardwareEnabledSet(bool enabled);

public:
	explicit Wifi(QObject* parent = nullptr);

	/// Set the state of the wifi software switch.
	Q_INVOKABLE void setEnabled(bool enabled);	

	[[nodiscard]] bool hardwareEnabled() const { return this->mHardwareEnabled; };
	[[nodiscard]] bool enabled() const { return this->mEnabled; };
	[[nodiscard]] UntypedObjectModel* devices() { return &this->mDevices; };
	[[nodiscard]] WifiDevice* defaultDevice() const { return this->mDefaultDevice; };
	void setDefaultDevice(WifiDevice* dev);

private:
	ObjectModel<WifiDevice> mDevices {this};
	WifiDevice* mDefaultDevice = nullptr;
	bool mEnabled = false;
	bool mHardwareEnabled = false;
};

}; // namespace qs::network
