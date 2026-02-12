#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qtypes.h>

namespace qs::network {

///! Connection state of a NetworkDevice.
class DeviceConnectionState: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		Unknown = 0,
		Connecting = 1,
		Connected = 2,
		Disconnecting = 3,
		Disconnected = 4,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(DeviceConnectionState::Enum state);
};

///! Type of network device.
class DeviceType: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		None = 0,
		Wifi = 1,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(DeviceType::Enum type);
};

///! NetworkManager-specific device state.
/// In sync with https://networkmanager.dev/docs/api/latest/nm-dbus-types.html#NMDeviceState.
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
	Q_INVOKABLE static QString toString(NMDeviceState::Enum state);
};

///! NetworkManager-specific device state change reasons.
/// In sync with https://networkmanager.dev/docs/api/latest/nm-dbus-types.html#NMDeviceStateReason.
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
	Q_INVOKABLE static QString toString(NMDeviceStateReason::Enum reason);
};

///! The connection state of a Network.
class NetworkState: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		Unknown = 0,
		Connecting = 1,
		Connected = 2,
		Disconnecting = 3,
		Disconnected = 4,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(NetworkState::Enum state);
};

///! The reason for the NMConnectionState of an NMConnection.
/// In sync with https://networkmanager.dev/docs/api/latest/nm-dbus-types.html#NMActiveConnectionStateReason.
class NMNetworkStateReason: public QObject {
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
	Q_INVOKABLE static QString toString(NMNetworkStateReason::Enum reason);
};

///! The security type of a wifi network.
class WifiSecurityType: public QObject {
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
		Open = 10,
		Unknown = 11,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(WifiSecurityType::Enum type);
};

///! The 802.11 mode of a wifi device.
class WifiDeviceMode: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		/// The device is part of an Ad-Hoc network without a central access point.
		AdHoc = 0,
		/// The device is a station that can connect to networks.
		Station = 1,
		/// The device is a local hotspot/access point.
		AccessPoint = 2,
		/// The device is an 802.11s mesh point.
		Mesh = 3,
		/// The device mode is unknown.
		Unknown = 4,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(WifiDeviceMode::Enum mode);
};

} // namespace qs::network
