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

QString NMDeviceStateReason::toString(NMDeviceStateReason::Enum reason) {
	switch (reason) {
	case None: return QStringLiteral("No reason");
	case Unknown: return QStringLiteral("Unknown error");
	case NowManaged: return QStringLiteral("Device is now managed");
	case NowUnmanaged: return QStringLiteral("Device is now unmanaged");
	case ConfigFailed: return QStringLiteral("The device could not be readied for configuration");
	case IpConfigUnavailable:
		return QStringLiteral(
		    "IP configuration could not be reserved (no available address, timeout, etc)"
		);
	case IpConfigExpired: return QStringLiteral("The IP config is no longer valid");
	case NoSecrets: return QStringLiteral("Secrets were required, but not provided");
	case SupplicantDisconnect: return QStringLiteral("802.1x supplicant disconnected");
	case SupplicantConfigFailed: return QStringLiteral("802.1x supplicant configuration failed");
	case SupplicantFailed: return QStringLiteral("802.1x supplicant failed");
	case SupplicantTimeout: return QStringLiteral("802.1x supplicant took too long to authenticate");
	case PppStartFailed: return QStringLiteral("PPP service failed to start");
	case PppDisconnect: return QStringLiteral("PPP service disconnected");
	case PppFailed: return QStringLiteral("PPP failed");
	case DhcpStartFailed: return QStringLiteral("DHCP client failed to start");
	case DhcpError: return QStringLiteral("DHCP client error");
	case DhcpFailed: return QStringLiteral("DHCP client failed");
	case SharedStartFailed: return QStringLiteral("Shared connection service failed to start");
	case SharedFailed: return QStringLiteral("Shared connection service failed");
	case AutoIpStartFailed: return QStringLiteral("AutoIP service failed to start");
	case AutoIpError: return QStringLiteral("AutoIP service error");
	case AutoIpFailed: return QStringLiteral("AutoIP service failed");
	case ModemBusy: return QStringLiteral("The line is busy");
	case ModemNoDialTone: return QStringLiteral("No dial tone");
	case ModemNoCarrier: return QStringLiteral("No carrier could be established");
	case ModemDialTimeout: return QStringLiteral("The dialing request timed out");
	case ModemDialFailed: return QStringLiteral("The dialing attempt failed");
	case ModemInitFailed: return QStringLiteral("Modem initialization failed");
	case GsmApnFailed: return QStringLiteral("Failed to select the specified APN");
	case GsmRegistrationNotSearching: return QStringLiteral("Not searching for networks");
	case GsmRegistrationDenied: return QStringLiteral("Network registration denied");
	case GsmRegistrationTimeout: return QStringLiteral("Network registration timed out");
	case GsmRegistrationFailed:
		return QStringLiteral("Failed to register with the requested network");
	case GsmPinCheckFailed: return QStringLiteral("PIN check failed");
	case FirmwareMissing: return QStringLiteral("Necessary firmware for the device may be missing");
	case Removed: return QStringLiteral("The device was removed");
	case Sleeping: return QStringLiteral("NetworkManager went to sleep");
	case ConnectionRemoved: return QStringLiteral("The device's active connection disappeared");
	case UserRequested: return QStringLiteral("Device disconnected by user or client");
	case Carrier: return QStringLiteral("Carrier/link changed");
	case ConnectionAssumed: return QStringLiteral("The device's existing connection was assumed");
	case SupplicantAvailable: return QStringLiteral("The supplicant is now available");
	case ModemNotFound: return QStringLiteral("The modem could not be found");
	case BtFailed: return QStringLiteral("The Bluetooth connection failed or timed out");
	case GsmSimNotInserted: return QStringLiteral("GSM Modem's SIM Card not inserted");
	case GsmSimPinRequired: return QStringLiteral("GSM Modem's SIM Pin required");
	case GsmSimPukRequired: return QStringLiteral("GSM Modem's SIM Puk required");
	case GsmSimWrong: return QStringLiteral("GSM Modem's SIM wrong");
	case InfinibandMode: return QStringLiteral("InfiniBand device does not support connected mode");
	case DependencyFailed: return QStringLiteral("A dependency of the connection failed");
	case Br2684Failed: return QStringLiteral("Problem with the RFC 2684 Ethernet over ADSL bridge");
	case ModemManagerUnavailable: return QStringLiteral("ModemManager not running");
	case SsidNotFound: return QStringLiteral("The Wi-Fi network could not be found");
	case SecondaryConnectionFailed:
		return QStringLiteral("A secondary connection of the base connection failed");
	case DcbFcoeFailed: return QStringLiteral("DCB or FCoE setup failed");
	case TeamdControlFailed: return QStringLiteral("teamd control failed");
	case ModemFailed: return QStringLiteral("Modem failed or no longer available");
	case ModemAvailable: return QStringLiteral("Modem now ready and available");
	case SimPinIncorrect: return QStringLiteral("SIM PIN was incorrect");
	case NewActivation: return QStringLiteral("New connection activation was enqueued");
	case ParentChanged: return QStringLiteral("The device's parent changed");
	case ParentManagedChanged: return QStringLiteral("The device parent's management changed");
	case OvsdbFailed: return QStringLiteral("Problem communicating with Open vSwitch database");
	case IpAddressDuplicate: return QStringLiteral("A duplicate IP address was detected");
	case IpMethodUnsupported: return QStringLiteral("The selected IP method is not supported");
	case SriovConfigurationFailed: return QStringLiteral("Configuration of SR-IOV parameters failed");
	case PeerNotFound: return QStringLiteral("The Wi-Fi P2P peer could not be found");
	case DeviceHandlerFailed:
		return QStringLiteral("The device handler dispatcher returned an error");
	case UnmanagedByDefault:
		return QStringLiteral(
		    "The device is unmanaged because the device type is unmanaged by default"
		);
	case UnmanagedExternalDown:
		return QStringLiteral(
		    "The device is unmanaged because it is an external device and is unconfigured (down or "
		    "without addresses)"
		);
	case UnmanagedLinkNotInit:
		return QStringLiteral("The device is unmanaged because the link is not initialized by udev");
	case UnmanagedQuitting:
		return QStringLiteral("The device is unmanaged because NetworkManager is quitting");
	case UnmanagedManagerDisabled:
		return QStringLiteral(
		    "The device is unmanaged because networking is disabled or the system is suspended"
		);
	case UnmanagedUserConf:
		return QStringLiteral("The device is unmanaged by user decision in NetworkManager.conf");
	case UnmanagedUserExplicit:
		return QStringLiteral("The device is unmanaged by explicit user decision");
	case UnmanagedUserSettings:
		return QStringLiteral("The device is unmanaged by user decision via settings plugin");
	case UnmanagedUserUdev: return QStringLiteral("The device is unmanaged via udev rule");
	case NetworkingOff: return QStringLiteral("NetworkManager was disabled (networking off)");
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
