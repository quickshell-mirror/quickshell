#include "wifi.hpp"
#include <utility>

#include <qdebug.h>
#include <qobject.h>
#include <qstring.h>

#include "device.hpp"
#include "network.hpp"

namespace qs::network {

QString WifiSecurityType::toString(WifiSecurityType::Enum type) {
	switch (type) {
	case Unknown: return QStringLiteral("Unknown");
	case Wpa3SuiteB192: return QStringLiteral("WPA3 Suite B 192-bit");
	case Sae: return QStringLiteral("WPA3");
	case Wpa2Eap: return QStringLiteral("WPA2 Enterprise");
	case Wpa2Psk: return QStringLiteral("WPA2");
	case WpaEap: return QStringLiteral("WPA Enterprise");
	case WpaPsk: return QStringLiteral("WPA");
	case StaticWep: return QStringLiteral("WEP");
	case DynamicWep: return QStringLiteral("Dynamic WEP");
	case Leap: return QStringLiteral("LEAP");
	case Owe: return QStringLiteral("OWE");
	case Open: return QStringLiteral("Open");
	default: return QStringLiteral("Unknown");
	}
}

QString WifiDeviceMode::toString(WifiDeviceMode::Enum mode) {
	switch (mode) {
	case Unknown: return QStringLiteral("Unknown");
	case AdHoc: return QStringLiteral("Ad-Hoc");
	case Station: return QStringLiteral("Station");
	case AccessPoint: return QStringLiteral("Access Point");
	case Mesh: return QStringLiteral("Mesh");
	default: return QStringLiteral("Unknown");
	};
}

WifiNetwork::WifiNetwork(QString ssid, QObject* parent): Network(std::move(ssid), parent) {};

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
