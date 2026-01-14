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

QString NMConnectionStateReason::toString(NMConnectionStateReason::Enum reason) {
	switch (reason) {
	case Unknown: return QStringLiteral("Unknown");
	case None: return QStringLiteral("No reason");
	case UserDisconnected: return QStringLiteral("User disconnection");
	case DeviceDisconnected:
		return QStringLiteral("The device the connection was using was disconnected.");
	case ServiceStopped:
		return QStringLiteral("The service providing the VPN connection was stopped.");
	case IpConfigInvalid:
		return QStringLiteral("The IP config of the active connection was invalid.");
	case ConnectTimeout:
		return QStringLiteral("The connection attempt to the VPN service timed out.");
	case ServiceStartTimeout:
		return QStringLiteral(
		    "A timeout occurred while starting the service providing the VPN connection."
		);
	case ServiceStartFailed:
		return QStringLiteral("Starting the service providing the VPN connection failed.");
	case NoSecrets: return QStringLiteral("Necessary secrets for the connection were not provided.");
	case LoginFailed: return QStringLiteral("Authentication to the server failed.");
	case ConnectionRemoved:
		return QStringLiteral("Necessary secrets for the connection were not provided.");
	case DependencyFailed:
		return QStringLiteral("Master connection of this connection failed to activate.");
	case DeviceRealizeFailed: return QStringLiteral("Could not create the software device link.");
	case DeviceRemoved: return QStringLiteral("The device this connection depended on disappeared.");
	default: return QStringLiteral("Unknown");
	};
};

WifiNetwork::WifiNetwork(QString ssid, QObject* parent): Network(std::move(ssid), parent) {
	this->bindablePasswordIsStatic().setBinding([this]() {
		switch (this->bSecurity) {
		case WifiSecurityType::StaticWep:
		case WifiSecurityType::WpaPsk:
		case WifiSecurityType::Wpa2Psk:
		case WifiSecurityType::Sae: return true;
		default: return false;
		}
	});
};

void WifiNetwork::connect() {
	if (this->bConnected) {
		qCCritical(logWifi) << this << "is already connected.";
		return;
	}

	this->requestConnect();
}

void WifiNetwork::connectWithPassword(const QString& password) {
	if (this->bConnected) {
		qCCritical(logWifi) << this << "is already connected.";
		return;
	}
	if (this->bKnown) {
		qCCritical(logWifi) << this
		                    << "is already known, attempting connection with the saved password.";
		this->requestConnect();
		return;
	}
	if (!this->bPasswordIsStatic) {
		qCCritical(
		    logWifi
		) << this
		  << "doesn't have a static password, attempting connection without a password.";
		this->requestConnect();
		return;
	}

	this->requestConnectWithPassword(password);
}

void WifiNetwork::disconnect() {
	if (!this->bConnected) {
		qCCritical(logWifi) << this << "is not currently connected";
		return;
	}

	this->requestDisconnect();
}

void WifiNetwork::forget() { this->requestForget(); }

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
