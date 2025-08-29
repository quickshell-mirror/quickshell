#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

namespace qs::network {

// 802.11 specific device encryption and authentication capabilities.
// In sync with https://networkmanager.dev/docs/api/latest/nm-dbus-types.html#NMDeviceWifiCapabilities.
class NMWirelessCapabilities: public QObject {
	Q_OBJECT;

public:
	enum Enum : quint16 {
		None = 0,
		CipherWep40 = 1,
		CipherWep104 = 2,
		CipherTkip = 4,
		CipherCcmp = 8,
		Wpa = 16,
		Rsn = 32,
		Ap = 64,
		Adhoc = 128,
		FreqValid = 256,
		Freq2Ghz = 512,
		Freq5Ghz = 1024,
		Freq6Ghz = 2048,
		Mesh = 4096,
		IbssRsn = 8192,
	};
	Q_ENUM(Enum);
};

class NMWirelessSecurityType: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		Wpa3SuiteB192 = 0,
		Sae = 1,
		Wpa2Eap = 2,
		Wpa2Psk = 3,
		WpaEap = 4,
		WpaPsk = 5,
		StaticWep = 6,
		DynamicWep = 7,
		Leap = 8,
		Owe = 9,
		None = 10,
		Unknown = 11,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(NMWirelessSecurityType::Enum type) {
		switch (type) {
		case Wpa3SuiteB192: return "WPA3 Suite B 192-bit";
		case Sae: return "WPA3";
		case Wpa2Eap: return "WPA2 Enterprise";
		case Wpa2Psk: return "WPA2";
		case WpaEap: return "WPA Enterprise";
		case WpaPsk: return "WPA";
		case StaticWep: return "WEP";
		case DynamicWep: return "Dynamic WEP";
		case Leap: return "LEAP";
		case Owe: return "OWE";
		case None: return "None";
		default: return "Unknown";
		}
	}
};

// In sync with https://networkmanager.dev/docs/api/latest/nm-dbus-types.html#NMDeviceState.
class NMDeviceState: public QObject {
	Q_OBJECT;

public:
	enum Enum : quint8 {
		Unknown = 0,
		Unmanaged = 10,
		Unavailable = 20,
		Disconnected = 30,
		Prepare = 40,
		Config = 50,
		NeedAuth = 60,
		IPConfig = 70,
		IPCheck = 80,
		Secondaries = 90,
		Activated = 100,
		Deactivating = 110,
		Failed = 120,
	};
	Q_ENUM(Enum);
};

// Indicates the 802.11 mode an access point is currently in.
// In sync with https://networkmanager.dev/docs/api/latest/nm-dbus-types.html#NM80211Mode.
class NM80211Mode: public QObject {
	Q_OBJECT;

public:
	enum Enum : quint8 {
		Unknown = 0,
		Adhoc = 1,
		Infra = 2,
		Ap = 3,
		Mesh = 3,
	};
	Q_ENUM(Enum);
};

// 802.11 access point flags.
// In sync with https://networkmanager.dev/docs/api/latest/nm-dbus-types.html#NM80211ApSecurityFlags.
class NM80211ApFlags: public QObject {
	Q_OBJECT;

public:
	enum Enum : quint8 {
		None = 0,
		Privacy = 1,
		Wps = 2,
		WpsPbc = 4,
		WpsPin = 8,
	};
	Q_ENUM(Enum);
};

// 802.11 access point security and authentication flags.
// These flags describe the current system requirements of an access point as determined from the access point's beacon.
// In sync with https://networkmanager.dev/docs/api/latest/nm-dbus-types.html#NM80211ApSecurityFlags.
class NM80211ApSecurityFlags: public QObject {
	Q_OBJECT;

public:
	enum Enum : quint16 {
		None = 0,
		PairWep40 = 1,
		PairWep104 = 2,
		PairTkip = 4,
		PairCcmp = 8,
		GroupWep40 = 16,
		GroupWep104 = 32,
		GroupTkip = 64,
		GroupCcmp = 128,
		KeyMgmtPsk = 256,
		KeyMgmt8021x = 512,
		KeyMgmtSae = 1024,
		KeyMgmtOwe = 2048,
		KeyMgmtOweTm = 4096,
		KeyMgmtEapSuiteB192 = 8192,
	};
	Q_ENUM(Enum);
};

// Indicates the state of a connection to a specific network while it is starting, connected, or disconnected from that network.
// In sync with https://networkmanager.dev/docs/api/latest/nm-dbus-types.html#NMActiveConnectionState.
class NMConnectionState: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		Unknown = 0,
		Activating = 1,
		Activated = 2,
		Deactivating = 3,
		Deactivated = 4
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(NMConnectionState::Enum state) {
		switch (state) {
		case Unknown: return "Unknown";
		case Activating: return "Activating";
		case Activated: return "Activated";
		case Deactivating: return "Deactivating";
		case Deactivated: return "Deactivated";
		}
	}
};

// Active connection state reasons.
// In sync with https://networkmanager.dev/docs/api/latest/nm-dbus-types.html#NMActiveConnectionStateReason.
class NMConnectionStateReason: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		Unknown = 0,
		None = 1,
		UserDisconnected = 2,
		DeviceDisconnected = 3,
		ServiceStopped = 4,
		IpConfigInvalid = 5,
		ConnectTimeout = 6,
		ServiceStartTimeout = 7,
		ServiceStartFailed = 8,
		NoSecrets = 9,
		LoginFailed = 10,
		ConnectionRemoved = 11,
		DependencyFailed = 12,
		DeviceRealizeFailed = 13,
		DeviceRemoved = 14
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(NMConnectionStateReason::Enum reason) {
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
};

} // namespace qs::network
