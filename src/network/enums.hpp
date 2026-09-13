#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qtypes.h>

namespace qs::network {

///! The degree to which the host can reach the internet.
class NetworkConnectivity: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		/// Network connectivity is unknown. This means the connectivity checks are disabled or have not run yet.
		Unknown = 0,
		/// The host is not connected to any network.
		None = 1,
		/// The internet connection is hijacked by a captive portal gateway.
		/// This indicates the shell should open a sandboxed web browser window for the purpose of authenticating to a gateway.
		Portal = 2,
		/// The host is connected to a network but does not appear to be able to reach the full internet.
		Limited = 3,
		/// The host is connected to a network and appears to be able to reach the full internet.
		Full = 4,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(NetworkConnectivity::Enum conn);
};

///! The backend supplying the Network service.
class NetworkBackendType: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		None = 0,
		NetworkManager = 1,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(NetworkBackendType::Enum type);
};

///! The connection state of a device or network.
class ConnectionState: public QObject {
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
	Q_INVOKABLE static QString toString(ConnectionState::Enum state);
};

///! The reason a connection failed.
class ConnectionFailReason: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		/// The connection failed for an unknown reason.
		Unknown = 0,
		/// Secrets were required, but not provided.
		NoSecrets = 1,
		/// The Wi-Fi supplicant disconnected.
		WifiClientDisconnected = 2,
		/// The Wi-Fi supplicant failed.
		WifiClientFailed = 3,
		/// The Wi-Fi connection took too long to authenticate.
		WifiAuthTimeout = 4,
		/// The Wi-Fi network could not be found.
		WifiNetworkLost = 5,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(ConnectionFailReason::Enum reason);
};

///! Type of a @@NetworkDevice.
class DeviceType: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		None = 0,
		Wifi = 1,
		Wired = 2,
	};
	Q_ENUM(Enum);
	Q_INVOKABLE static QString toString(DeviceType::Enum type);
};

///! The security type of a @@WifiNetwork.
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

///! The 802.11 mode of a @@WifiDevice.
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
