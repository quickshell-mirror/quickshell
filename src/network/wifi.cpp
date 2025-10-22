#include "wifi.hpp"
#include <utility>

#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qstringliteral.h>
#include <qtmetamacros.h>

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

QString NMConnectionStateReason::toString(NMConnectionStateReason::Enum reason) {
	switch (reason) {
	case Unknown: return "Unknown";
	case None: return "No reason";
	case UserDisconnected: return "User disconnection";
	case DeviceDisconnected: return "The device the connection was using was disconnected.";
	case ServiceStopped: return "The service providing the VPN connection was stopped.";
	case IpConfigInvalid: return "The IP config of the active connection was invalid.";
	case ConnectTimeout: return "The connection attempt to the VPN service timed out.";
	case ServiceStartTimeout:
		return "A timeout occurred while starting the service providing the VPN connection.";
	case ServiceStartFailed: return "Starting the service providing the VPN connection failed.";
	case NoSecrets: return "Necessary secrets for the connection were not provided.";
	case LoginFailed: return "Authentication to the server failed.";
	case ConnectionRemoved: return "Necessary secrets for the connection were not provided.";
	case DependencyFailed: return " Master connection of this connection failed to activate.";
	case DeviceRealizeFailed: return "Could not create the software device link.";
	case DeviceRemoved: return "The device this connection depended on disappeared.";
	};
};

WifiScanner::WifiScanner(QObject* parent): QObject(parent) {};

void WifiScanner::setEnabled(bool enabled) {
	if (this->bEnabled == enabled) {
		const QString state = enabled ? "enabled" : "disabled";
		qCCritical(logWifi) << "Scanner is already" << state;
	} else {
		this->bEnabled = enabled;
		emit this->requestEnabled(enabled);
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
