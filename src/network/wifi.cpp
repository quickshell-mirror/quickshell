#include "wifi.hpp"
#include <utility>

#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qstringliteral.h>

#include "../core/logcat.hpp"
#include "device.hpp"
#include "network.hpp"

namespace qs::network {

namespace {
QS_LOGGING_CATEGORY(logWifi, "quickshell.network.wifi", QtWarningMsg);
} // namespace

QString WifiSecurityType::toString(WifiSecurityType::Enum type) {
	switch (type) {
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
	case Unknown: return QStringLiteral("Unknown");
	}
}

WifiNetwork::WifiNetwork(QString ssid, QObject* parent): BaseNetwork(std::move(ssid), parent) {};

void WifiNetwork::connect() {
	if (this->bConnected) {
		qCCritical(logWifi) << this << "is already connected.";
		return;
	}
	this->requestConnect();
}

WifiDevice::WifiDevice(QObject* parent): NetworkDevice(DeviceType::Wifi, parent) {};

void WifiDevice::scan() {
	if (this->bScanning) {
		qCCritical(logWifi) << this << "is already scanning";
		return;
	}
	qCDebug(logWifi) << "Requesting scan on" << this;
	this->requestScan();
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
