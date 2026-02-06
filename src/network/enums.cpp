#include "enums.hpp"

#include <qstring.h>

namespace qs::network {

QString DeviceConnectionState::toString(DeviceConnectionState::Enum state) {
	switch (state) {
	case Unknown: return QStringLiteral("Unknown");
	case Connecting: return QStringLiteral("Connecting");
	case Connected: return QStringLiteral("Connected");
	case Disconnecting: return QStringLiteral("Disconnecting");
	case Disconnected: return QStringLiteral("Disconnected");
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

QString NMDeviceState::toString(NMDeviceState::Enum state) {
	switch (state) {
	case Unknown: return QStringLiteral("Unknown");
	case Unmanaged: return QStringLiteral("Not managed by NetworkManager");
	case Unavailable: return QStringLiteral("Unavailable");
	case Disconnected: return QStringLiteral("Disconnected");
	case Prepare: return QStringLiteral("Preparing to connect");
	case Config: return QStringLiteral("Connecting to a network");
	case NeedAuth: return QStringLiteral("Waiting for authentication");
	case IPConfig: return QStringLiteral("Requesting IPv4 and/or IPv6 addresses from the network");
	case IPCheck:
		return QStringLiteral("Checking if further action is required for the requested connection");
	case Secondaries:
		return QStringLiteral("Waiting for a required secondary connection to activate");
	case Activated: return QStringLiteral("Connected");
	case Deactivating: return QStringLiteral("Disconnecting");
	case Failed: return QStringLiteral("Failed to connect");
	default: return QStringLiteral("Unknown");
	};
}

QString NetworkState::toString(NetworkState::Enum state) {
	switch (state) {
	case NetworkState::Connecting: return QStringLiteral("Connecting");
	case NetworkState::Connected: return QStringLiteral("Connected");
	case NetworkState::Disconnecting: return QStringLiteral("Disconnecting");
	case NetworkState::Disconnected: return QStringLiteral("Disconnected");
	default: return QStringLiteral("Unknown");
	}
}

QString NMNetworkStateReason::toString(NMNetworkStateReason::Enum reason) {
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
