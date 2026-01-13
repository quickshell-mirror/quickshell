#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/model.hpp"
#include "device.hpp"
#include "network.hpp"

namespace qs::network {

///! The security type of a wifi network.
class WifiSecurityType: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		Wpa3SuiteB192 = 0,
		Sae = 1,
		Wpa2Eap = 2,
		Wpa2Psk = 3,
		WpaEap = 4,
		WpaPsk = 5,
		StaticWep = 6,
		DynamicWep = 7,
		Leap = 8,
		Owe = 9,
		Open = 10,
		Unknown = 11,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(WifiSecurityType::Enum type);
};

///! The 802.11 mode of a wifi device.
class WifiDeviceMode: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		/// The device is part of an Ad-Hoc network without a central access point.
		AdHoc = 0,
		/// The device is a station that can connect to networks.
		Station = 1,
		/// The device is a local hotspot/access point.
		AccessPoint = 2,
		/// The device is an 802.11s mesh point.
		Mesh = 3,
		/// The device mode is unknown.
		Unknown = 4,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(WifiDeviceMode::Enum mode);
};

///! NetworkManager-specific reason for a WifiNetworks connection state.
/// In sync with https://networkmanager.dev/docs/api/latest/nm-dbus-types.html#NMActiveConnectionStateReason.
class NMConnectionStateReason: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		Unknown = 0,
		None = 1,
		UserDisconnected = 2,
		DeviceDisconnected = 3,
		ServiceStopped = 4,
		IpConfigInvalid = 5,
		ConnectTimeout = 6,
		ServiceStartTimeout = 7,
		ServiceStartFailed = 8,
		NoSecrets = 9,
		LoginFailed = 10,
		ConnectionRemoved = 11,
		DependencyFailed = 12,
		DeviceRealizeFailed = 13,
		DeviceRemoved = 14
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(NMConnectionStateReason::Enum reason);
};

///! An available wifi network.
class WifiNetwork: public Network {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("WifiNetwork can only be acquired through WifiDevice");
	// clang-format off
	/// The current signal strength of the network, from 0.0 to 1.0.
	Q_PROPERTY(qreal signalStrength READ default NOTIFY signalStrengthChanged BINDABLE bindableSignalStrength);
	/// True if the wifi network has known connection settings saved.
	Q_PROPERTY(bool known READ default NOTIFY knownChanged BINDABLE bindableKnown);
	/// The security type of the wifi network.
	Q_PROPERTY(WifiSecurityType::Enum security READ default NOTIFY securityChanged BINDABLE bindableSecurity);
	/// A specific reason for the connection state when the backend is NetworkManager.
	Q_PROPERTY(NMConnectionStateReason::Enum nmReason READ default NOTIFY nmReasonChanged BINDABLE bindableNmReason);
	/// True if the wifi password is static. This implies that you can supply a password to connect().
	Q_PROPERTY(bool passwordIsStatic READ default NOTIFY passwordIsStaticChanged BINDABLE bindablePasswordIsStatic);
	// clang-format on

public:
	explicit WifiNetwork(QString ssid, QObject* parent = nullptr);

	/// Attempt to connect to the wifi network.
	///
	/// > [!WARNING] Quickshell does not yet provide a NetworkManager authentication agent,
	/// > meaning another agent will need to be active to enter passwords for unsaved networks.
	Q_INVOKABLE void connect(const QString& password = QString());
	/// Disconnect from the wifi network.
	Q_INVOKABLE void disconnect();
	/// Forget all connection settings for this wifi network.
	Q_INVOKABLE void forget();

	QBindable<qreal> bindableSignalStrength() { return &this->bSignalStrength; }
	QBindable<bool> bindableKnown() { return &this->bKnown; }
	QBindable<NMConnectionStateReason::Enum> bindableNmReason() { return &this->bNmReason; }
	QBindable<WifiSecurityType::Enum> bindableSecurity() { return &this->bSecurity; }
	QBindable<bool> bindablePasswordIsStatic() { return &this->bPasswordIsStatic; }

signals:
	void requestConnect(QString password);
	void requestDisconnect();
	void requestForget();
	void signalStrengthChanged();
	void knownChanged();
	void securityChanged();
	void nmReasonChanged();
	void passwordIsStaticChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, qreal, bSignalStrength, &WifiNetwork::signalStrengthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, bool, bKnown, &WifiNetwork::knownChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, NMConnectionStateReason::Enum, bNmReason, &WifiNetwork::nmReasonChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, WifiSecurityType::Enum, bSecurity, &WifiNetwork::securityChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, bool, bPasswordIsStatic, &WifiNetwork::passwordIsStaticChanged);
	// clang-format on
};

///! Wireless variant of a NetworkDevice.
class WifiDevice: public NetworkDevice {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");

	// clang-format off
	/// A list of this available and connected wifi networks.
	QSDOC_TYPE_OVERRIDE(ObjectModel<WifiNetwork>*);
	Q_PROPERTY(UntypedObjectModel* networks READ networks CONSTANT);
	/// True when currently scanning for networks.
	/// When enabled, the scanner populates the device with an active list of available wifi networks.
	Q_PROPERTY(bool scannerEnabled READ scannerEnabled WRITE setScannerEnabled NOTIFY scannerEnabledChanged BINDABLE bindableScannerEnabled);
	/// The 802.11 mode the device is in.
	Q_PROPERTY(WifiDeviceMode::Enum mode READ default NOTIFY modeChanged BINDABLE bindableMode);
	// clang-format on

public:
	explicit WifiDevice(QObject* parent = nullptr);

	void networkAdded(WifiNetwork* net);
	void networkRemoved(WifiNetwork* net);

	[[nodiscard]] ObjectModel<WifiNetwork>* networks() { return &this->mNetworks; };
	QBindable<bool> bindableScannerEnabled() { return &this->bScannerEnabled; };
	[[nodiscard]] bool scannerEnabled() const { return this->bScannerEnabled; };
	void setScannerEnabled(bool enabled);
	QBindable<WifiDeviceMode::Enum> bindableMode() { return &this->bMode; }

signals:
	void modeChanged();
	void scannerEnabledChanged(bool enabled);

private:
	ObjectModel<WifiNetwork> mNetworks {this};
	Q_OBJECT_BINDABLE_PROPERTY(WifiDevice, bool, bScannerEnabled, &WifiDevice::scannerEnabledChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiDevice, WifiDeviceMode::Enum, bMode, &WifiDevice::modeChanged);
};

}; // namespace qs::network

QDebug operator<<(QDebug debug, const qs::network::WifiNetwork* network);
QDebug operator<<(QDebug debug, const qs::network::WifiDevice* device);
