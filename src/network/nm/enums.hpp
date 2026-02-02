#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

namespace qs::network {

// In sync with https://networkmanager.dev/docs/api/latest/nm-dbus-types.html#NMConnectivityState
class NMConnectivityState: public QObject {
	Q_OBJECT;

public:
	enum Enum : quint8 {
		Unknown = 0,
		None = 1,
		Portal = 2,
		Limited = 3,
		Full = 4,
	};
};

// Indicates the type of hardware represented by a device object.
class NMDeviceType: public QObject {
	Q_OBJECT;

public:
	enum Enum : quint8 {
		Unknown = 0,
		Ethernet = 1,
		Wifi = 2,
		Unused1 = 3,
		Unused2 = 4,
		Bluetooth = 5,
		OlpcMesh = 6,
		Wimax = 7,
		Modem = 8,
		InfiniBand = 9,
		Bond = 10,
		Vlan = 11,
		Adsl = 12,
		Bridge = 13,
		Generic = 14,
		Team = 15,
		Tun = 16,
		IpTunnel = 17,
		MacVlan = 18,
		VxLan = 19,
		Veth = 20,
		MacSec = 21,
		Dummy = 22,
		Ppp = 23,
		OvsInterface = 24,
		OvsPort = 25,
		OvsBridge = 26,
		Wpan = 27,
		Lowpan = 28,
		Wireguard = 29,
		WifiP2P = 30,
		Vrf = 31,
		Loopback = 32,
		Hsr = 33,
		IpVlan = 34,
	};
	Q_ENUM(Enum);
};

// In sync with https://networkmanager.dev/docs/api/latest/nm-dbus-types.html#NMDeviceState.
class NMDeviceState: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

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

// Device state change reason codes.
// In sync with https://networkmanager.dev/docs/api/latest/nm-dbus-types.html#NMDeviceStateReason.
class NMDeviceStateReason: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		None = 0,
		Unknown = 1,
		NowManaged = 2,
		NowUnmanaged = 3,
		ConfigFailed = 4,
		IpConfigUnavailable = 5,
		IpConfigExpired = 6,
		NoSecrets = 7,
		SupplicantDisconnect = 8,
		SupplicantConfigFailed = 9,
		SupplicantFailed = 10,
		SupplicantTimeout = 11,
		PppStartFailed = 12,
		PppDisconnect = 13,
		PppFailed = 14,
		DhcpStartFailed = 15,
		DhcpError = 16,
		DhcpFailed = 17,
		SharedStartFailed = 18,
		SharedFailed = 19,
		AutoIpStartFailed = 20,
		AutoIpError = 21,
		AutoIpFailed = 22,
		ModemBusy = 23,
		ModemNoDialTone = 24,
		ModemNoCarrier = 25,
		ModemDialTimeout = 26,
		ModemDialFailed = 27,
		ModemInitFailed = 28,
		GsmApnFailed = 29,
		GsmRegistrationNotSearching = 30,
		GsmRegistrationDenied = 31,
		GsmRegistrationTimeout = 32,
		GsmRegistrationFailed = 33,
		GsmPinCheckFailed = 34,
		FirmwareMissing = 35,
		Removed = 36,
		Sleeping = 37,
		ConnectionRemoved = 38,
		UserRequested = 39,
		Carrier = 40,
		ConnectionAssumed = 41,
		SupplicantAvailable = 42,
		ModemNotFound = 43,
		BtFailed = 44,
		GsmSimNotInserted = 45,
		GsmSimPinRequired = 46,
		GsmSimPukRequired = 47,
		GsmSimWrong = 48,
		InfinibandMode = 49,
		DependencyFailed = 50,
		Br2684Failed = 51,
		ModemManagerUnavailable = 52,
		SsidNotFound = 53,
		SecondaryConnectionFailed = 54,
		DcbFcoeFailed = 55,
		TeamdControlFailed = 56,
		ModemFailed = 57,
		ModemAvailable = 58,
		SimPinIncorrect = 59,
		NewActivation = 60,
		ParentChanged = 61,
		ParentManagedChanged = 62,
		OvsdbFailed = 63,
		IpAddressDuplicate = 64,
		IpMethodUnsupported = 65,
		SriovConfigurationFailed = 66,
		PeerNotFound = 67,
		DeviceHandlerFailed = 68,
		UnmanagedByDefault = 69,
		UnmanagedExternalDown = 70,
		UnmanagedLinkNotInit = 71,
		UnmanagedQuitting = 72,
		UnmanagedManagerDisabled = 73,
		UnmanagedUserConf = 74,
		UnmanagedUserExplicit = 75,
		UnmanagedUserSettings = 76,
		UnmanagedUserUdev = 77,
		NetworkingOff = 78,
	};
	Q_ENUM(Enum);
};

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
		Mesh = 4,
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

public:
	enum Enum : quint8 {
		Unknown = 0,
		Activating = 1,
		Activated = 2,
		Deactivating = 3,
		Deactivated = 4
	};
	Q_ENUM(Enum);
};

/// In sync with https://networkmanager.dev/docs/api/latest/nm-dbus-types.html#NMActiveConnectionStateReason.
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
};

} // namespace qs::network
