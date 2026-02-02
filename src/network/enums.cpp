#include "enums.hpp"

#include <qstring.h>

namespace qs::network {

QString NetworkConnectivity::toString(NetworkConnectivity::Enum conn) {
	switch (conn) {
	case Unknown: return QStringLiteral("Unknown");
	case None: return QStringLiteral("Not connected to a network");
	case Portal: return QStringLiteral("Connection intercepted by a captive portal");
	case Limited: return QStringLiteral("Partial internet connectivity");
	case Full: return QStringLiteral("Full internet connectivity");
	default: return QStringLiteral("Unknown");
	}
}

QString NetworkBackendType::toString(NetworkBackendType::Enum type) {
	switch (type) {
	case NetworkBackendType::None: return "None";
	case NetworkBackendType::NetworkManager: return "NetworkManager";
	default: return "Unknown";
	}
}

QString ConnectionState::toString(ConnectionState::Enum state) {
	switch (state) {
	case Unknown: return QStringLiteral("Unknown");
	case Connecting: return QStringLiteral("Connecting");
	case Connected: return QStringLiteral("Connected");
	case Disconnecting: return QStringLiteral("Disconnecting");
	case Disconnected: return QStringLiteral("Disconnected");
	default: return QStringLiteral("Unknown");
	}
}

QString ConnectionFailReason::toString(ConnectionFailReason::Enum reason) {
	switch (reason) {
	case Unknown: return QStringLiteral("Unknown");
	case NoSecrets: return QStringLiteral("Secrets were required but not provided");
	case WifiClientDisconnected: return QStringLiteral("Wi-Fi supplicant diconnected");
	case WifiClientFailed: return QStringLiteral("Wi-Fi supplicant failed");
	case WifiAuthTimeout: return QStringLiteral("Wi-Fi connection took too long to authenticate");
	case WifiNetworkLost: return QStringLiteral("Wi-Fi network could not be found");
	default: return QStringLiteral("Unknown");
	}
}

QString DeviceType::toString(DeviceType::Enum type) {
	switch (type) {
	case None: return QStringLiteral("None");
	case Wifi: return QStringLiteral("Wifi");
	default: return QStringLiteral("Unknown");
	}
}

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

} // namespace qs::network
