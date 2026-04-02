#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/doc.hpp"
#include "device.hpp"
#include "enums.hpp"
#include "network.hpp"

namespace qs::network {

///! WiFi subtype of @@Network.
class WifiNetwork: public Network {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("WifiNetwork can only be acquired through WifiDevice");
	// clang-format off
	/// The current signal strength of the network, from 0.0 to 1.0.
	Q_PROPERTY(qreal signalStrength READ default NOTIFY signalStrengthChanged BINDABLE bindableSignalStrength);
	/// The security type of the wifi network.
	Q_PROPERTY(WifiSecurityType::Enum security READ default NOTIFY securityChanged BINDABLE bindableSecurity);
	// clang-format on

public:
	explicit WifiNetwork(QString ssid, NetworkDevice* device, QObject* parent = nullptr);
	/// Attempt to connect to the network with the given PSK. If the PSK is wrong,
	/// a @@Network.connectionFailed(s) signal will be emitted with `NoSecrets`.
	///
	/// The networking backend may store the PSK for future use with @@Network.connect().
	/// As such, calling that function first is recommended to avoid having to show a
	/// prompt if not required.
	///
	/// > [!NOTE] PSKs should only be provided when the @@security is one of
	/// > `WpaPsk`, `Wpa2Psk`, or `Sae`.
	Q_INVOKABLE void connectWithPsk(const QString& psk);

	QBindable<qreal> bindableSignalStrength() { return &this->bSignalStrength; }
	QBindable<WifiSecurityType::Enum> bindableSecurity() { return &this->bSecurity; }

signals:
	QSDOC_HIDE void requestConnectWithPsk(QString psk);
	void signalStrengthChanged();
	void securityChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, qreal, bSignalStrength, &WifiNetwork::signalStrengthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, WifiSecurityType::Enum, bSecurity, &WifiNetwork::securityChanged);
	// clang-format on
};

///! WiFi variant of a @@NetworkDevice.
class WifiDevice: public NetworkDevice {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");

	// clang-format off
	/// True when currently scanning for networks.
	/// When enabled, the scanner populates the device with an active list of available wifi networks.
	Q_PROPERTY(bool scannerEnabled READ scannerEnabled WRITE setScannerEnabled NOTIFY scannerEnabledChanged BINDABLE bindableScannerEnabled);
	/// The 802.11 mode the device is in.
	Q_PROPERTY(WifiDeviceMode::Enum mode READ default NOTIFY modeChanged BINDABLE bindableMode);
	// clang-format on

public:
	explicit WifiDevice(QObject* parent = nullptr);

	QBindable<bool> bindableScannerEnabled() { return &this->bScannerEnabled; }
	[[nodiscard]] bool scannerEnabled() const { return this->bScannerEnabled; }
	void setScannerEnabled(bool enabled);
	QBindable<WifiDeviceMode::Enum> bindableMode() { return &this->bMode; }

signals:
	void modeChanged();
	void scannerEnabledChanged(bool enabled);

private:
	Q_OBJECT_BINDABLE_PROPERTY(WifiDevice, bool, bScannerEnabled, &WifiDevice::scannerEnabledChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiDevice, WifiDeviceMode::Enum, bMode, &WifiDevice::modeChanged);
};

} // namespace qs::network

QDebug operator<<(QDebug debug, const qs::network::WifiNetwork* network);
QDebug operator<<(QDebug debug, const qs::network::WifiDevice* device);
