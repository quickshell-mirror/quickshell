#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/model.hpp"
#include "device.hpp"
#include "network.hpp"
#include "nm/enums.hpp"

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
	QBindable<bool> bindableScanning() { return &this->bScanning; };
	[[nodiscard]] bool scanning() const { return this->bScanning; };

private:
	ObjectModel<WifiNetwork> mNetworks {this};
	Q_OBJECT_BINDABLE_PROPERTY(WifiDevice, bool, bScanning, &WifiDevice::scanningChanged);
};
}; // namespace qs::network

QDebug operator<<(QDebug debug, const qs::network::WifiNetwork* network);
QDebug operator<<(QDebug debug, const qs::network::WifiDevice* device);
