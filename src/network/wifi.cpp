#include "wifi.hpp"
#include <utility>

#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtmetamacros.h>

#include "../core/logcat.hpp"
#include "device.hpp"
#include "enums.hpp"
#include "network.hpp"

namespace qs::network {

namespace {
QS_LOGGING_CATEGORY(logWifiNetwork, "quickshell.wifinetwork", QtWarningMsg);
}

WifiNetwork::WifiNetwork(QString ssid, QObject* parent): Network(std::move(ssid), parent) {};

void WifiNetwork::connectWithPsk(const QString& psk) {
	if (this->bConnected) {
		qCCritical(logWifiNetwork) << this << "is already connected.";
		return;
	}
	if (this->bSecurity != WifiSecurityType::WpaPsk && this->bSecurity != WifiSecurityType::Wpa2Psk
	    && this->bSecurity != WifiSecurityType::Sae)
	{
		qCCritical(logWifiNetwork) << this << "has the wrong security type for a PSK.";
		return;
	}
	emit this->requestConnectWithPsk(psk);
}

WifiDevice::WifiDevice(QObject* parent): NetworkDevice(DeviceType::Wifi, parent) {};

void WifiDevice::setScannerEnabled(bool enabled) {
	if (this->bScannerEnabled == enabled) return;
	this->bScannerEnabled = enabled;
}

void WifiDevice::networkAdded(WifiNetwork* net) { this->mNetworks.insertObject(net); }
void WifiDevice::networkRemoved(WifiNetwork* net) { this->mNetworks.removeObject(net); }

} // namespace qs::network

QDebug operator<<(QDebug debug, const qs::network::WifiNetwork* network) {
	auto saver = QDebugStateSaver(debug);

	if (network) {
		debug.nospace() << "WifiNetwork(" << static_cast<const void*>(network)
		                << ", name=" << network->name() << ")";
	} else {
		debug << "WifiNetwork(nullptr)";
	}

	return debug;
}

QDebug operator<<(QDebug debug, const qs::network::WifiDevice* device) {
	auto saver = QDebugStateSaver(debug);

	if (device) {
		debug.nospace() << "WifiDevice(" << static_cast<const void*>(device)
		                << ", name=" << device->name() << ")";
	} else {
		debug << "WifiDevice(nullptr)";
	}

	return debug;
}
