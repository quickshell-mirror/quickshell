#pragma once

#include <qdatetime.h>
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
	/// The radio frequency of the access point in MHz.
	Q_PROPERTY(quint32 frequency READ default NOTIFY frequencyChanged BINDABLE bindableFrequency);
	/// The hardware address (BSSID) of the access point.
	Q_PROPERTY(QString bssid READ default NOTIFY bssidChanged BINDABLE bindableBssid);
	/// The maximum bitrate this access point is capable of, in kilobits/second.
	Q_PROPERTY(quint32 maxBitrate READ default NOTIFY maxBitrateChanged BINDABLE bindableMaxBitrate);
	/// The channel bandwidth announced by the access point in MHz (e.g. 20, 40, 80, 160).
	Q_PROPERTY(quint32 bandwidth READ default NOTIFY bandwidthChanged BINDABLE bindableBandwidth);
	/// The date and time when the access point was last seen in scan results.
	Q_PROPERTY(QDateTime lastSeen READ default NOTIFY lastSeenChanged BINDABLE bindableLastSeen);
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
	QBindable<quint32> bindableFrequency() { return &this->bFrequency; }
	QBindable<QString> bindableBssid() { return &this->bBssid; }
	QBindable<quint32> bindableMaxBitrate() { return &this->bMaxBitrate; }
	QBindable<quint32> bindableBandwidth() { return &this->bBandwidth; }
	QBindable<QDateTime> bindableLastSeen() { return &this->bLastSeen; }

signals:
	QSDOC_HIDE void requestConnectWithPsk(QString psk);
	void signalStrengthChanged();
	void securityChanged();
	void frequencyChanged();
	void bssidChanged();
	void maxBitrateChanged();
	void bandwidthChanged();
	void lastSeenChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, qreal, bSignalStrength, &WifiNetwork::signalStrengthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, WifiSecurityType::Enum, bSecurity, &WifiNetwork::securityChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, quint32, bFrequency, &WifiNetwork::frequencyChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, QString, bBssid, &WifiNetwork::bssidChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, quint32, bMaxBitrate, &WifiNetwork::maxBitrateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, quint32, bBandwidth, &WifiNetwork::bandwidthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WifiNetwork, QDateTime, bLastSeen, &WifiNetwork::lastSeenChanged);
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
