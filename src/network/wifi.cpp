#include "wifi.hpp"

#include <qlogging.h>
#include <qloggingcategory.h>

#include "../core/logcat.hpp"

namespace qs::network {

namespace {
QS_LOGGING_CATEGORY(logWifiDevice, "quickshell.wifi.device", QtWarningMsg);
QS_LOGGING_CATEGORY(logWifiNetwork, "quickshell.wifi.network", QtWarningMsg);
QS_LOGGING_CATEGORY(logWifi, "quickshell.wifi", QtWarningMsg);
} // namespace

WifiDevice::WifiDevice(QObject* parent): NetworkDevice(parent) {};

void WifiDevice::scan() {
	if (this->bScanning) {
		qCCritical(logWifiDevice) << this << "is already scanning";
		return;
	}
	qCDebug(logWifiDevice) << "Requesting scan on" << this;
	this->requestScan();
}

void WifiDevice::networkAdded(WifiNetwork* net) { this->mNetworks.insertObject(net); }
void WifiDevice::networkRemoved(WifiNetwork* net) { this->mNetworks.removeObject(net); }

WifiNetwork::WifiNetwork(QString ssid, QObject* parent): QObject(parent), mSsid(std::move(ssid)) {};

void WifiNetwork::connect() {
	if (this->bConnected) {
		qCCritical(logWifiNetwork) << this << "is already connected.";
		return;
	}
	this->requestConnect();
}

Wifi::Wifi(QObject* parent): QObject(parent) {};

void Wifi::onDeviceAdded(WifiDevice* dev) { this->mDevices.insertObject(dev); }
void Wifi::onDeviceRemoved(WifiDevice* dev) { this->mDevices.removeObject(dev); }

void Wifi::setEnabled(bool enabled) {
	if (this->bEnabled == enabled) {
		QString state = enabled ? "enabled" : "disabled";
		qCCritical(logWifi) << "Wifi is already" << state;
	} else {
		emit this->requestSetEnabled(enabled);
	}
}

} // namespace qs::network

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

QDebug operator<<(QDebug debug, const qs::network::WifiNetwork* network) {
	auto saver = QDebugStateSaver(debug);

	if (network) {
		debug.nospace() << "WifiNetwork(" << static_cast<const void*>(network)
		                << ", name=" << network->ssid() << ")";
	} else {
		debug << "WifiDevice(nullptr)";
	}

	return debug;
}
