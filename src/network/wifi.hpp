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

///! Scans for available wifi networks.
/// When enabled, the scanner populates its respective WifiDevice with an active list of available WifiNetworks.
class WifiScanner: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("Scanner can only be acquired through WifiDevice");

	// clang-format off
	/// True when currently scanning for networks.
	Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged BINDABLE bindableEnabled);
	// clang-format on

public:
	explicit WifiScanner(QObject* parent = nullptr);

	QBindable<bool> bindableEnabled() { return &this->bEnabled; };
	[[nodiscard]] bool enabled() const { return this->bEnabled; };
	void setEnabled(bool enabled);

signals:
	void requestEnabled(bool enabled);
	void enabledChanged();

private:
	Q_OBJECT_BINDABLE_PROPERTY(WifiScanner, bool, bEnabled, &WifiScanner::enabledChanged);
};

///! An available wifi network.
class WifiNetwork: public BaseNetwork {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("WifiNetwork can only be acquired through WifiDevice");
	// clang-format off
	/// The current signal strength of the network, from 0.0 to 1.0.
	Q_PROPERTY(qreal signalStrength READ signalStrength NOTIFY signalStrengthChanged BINDABLE bindableSignalStrength);
	/// True if the wifi network has known connection settings saved.
	Q_PROPERTY(bool known READ known NOTIFY knownChanged BINDABLE bindableKnown);
	/// The security type of the wifi network.
	Q_PROPERTY(WifiSecurityType::Enum security READ security NOTIFY securityChanged BINDABLE bindableSecurity);
	/// A specific reason for the connection state when the backend is NetworkManager.
	Q_PROPERTY(NMConnectionStateReason::Enum nmReason READ nmReason NOTIFY nmReasonChanged BINDABLE bindableNmReason);
	// clang-format on

public:
	explicit WifiNetwork(QString ssid, QObject* parent = nullptr);

	/// Attempt to connect to the wifi network.
	Q_INVOKABLE void connect();

	QBindable<qreal> bindableSignalStrength() { return &this->bSignalStrength; }
	[[nodiscard]] qreal signalStrength() const { return this->bSignalStrength; };
	QBindable<bool> bindableKnown() { return &this->bKnown; }
	[[nodiscard]] bool known() const { return this->bKnown; };
	QBindable<NMConnectionStateReason::Enum> bindableNmReason() { return &this->bNmReason; }
	[[nodiscard]] NMConnectionStateReason::Enum nmReason() const { return this->bNmReason; };
	QBindable<WifiSecurityType::Enum> bindableSecurity() { return &this->bSecurity; }
	[[nodiscard]] WifiSecurityType::Enum security() const { return this->bSecurity; };

signals:
	void requestConnect();
	void signalStrengthChanged();
	void knownChanged();
	void securityChanged();
	void nmReasonChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, qreal, bSignalStrength, &WifiNetwork::signalStrengthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, bool, bKnown, &WifiNetwork::knownChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, NMConnectionStateReason::Enum, bNmReason, &WifiNetwork::nmReasonChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, WifiSecurityType::Enum, bSecurity, &WifiNetwork::securityChanged);
	// clang-format on
};

///! Wireless variant of a NetworkDevice.
class WifiDevice: public NetworkDevice {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("WifiDevices can only be acquired through Wifi");

	/// A list of this available and connected wifi networks.
	Q_PROPERTY(UntypedObjectModel* networks READ networks CONSTANT);
	QSDOC_TYPE_OVERRIDE(ObjectModel<WifiNetwork>*)
	/// The wifi scanner for this device.
	Q_PROPERTY(WifiScanner* wifiScanner READ wifiScanner CONSTANT);

public slots:
	void networkAdded(WifiNetwork* net);
	void networkRemoved(WifiNetwork* net);

public:
	explicit WifiDevice(QObject* parent = nullptr);

	[[nodiscard]] ObjectModel<WifiNetwork>* networks() { return &this->mNetworks; };
	[[nodiscard]] WifiScanner* wifiScanner() { return &this->mScanner; };

private:
	ObjectModel<WifiNetwork> mNetworks {this};
	WifiScanner mScanner;
};

}; // namespace qs::network

QDebug operator<<(QDebug debug, const qs::network::WifiNetwork* network);
QDebug operator<<(QDebug debug, const qs::network::WifiDevice* device);
