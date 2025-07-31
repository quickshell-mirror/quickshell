#pragma once

#include <qcontainerfwd.h>
#include <qdbusextratypes.h>
#include <qdbusservicewatcher.h>
#include <qhash.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

namespace qs::network {

// 802.11 specific device encryption and authentication capabilities.
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

class NMDeviceState: public QObject {
	Q_OBJECT;

public:
	enum Enum : quint8 {
		Unkown = 0,
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

// Indicates the type of hardware represented by a device object
class NMDeviceType: public QObject {
	Q_OBJECT;

public:
	enum Enum : quint8 {
		Unknown = 0,
		Generic = 14,
		Ethernet = 1,
		Wifi = 2,
		Unused1 = 3,
		Unused2 = 4,
		Bluetooth = 5,
		OlpcMesh = 6,
		Wimax = 7,
		Modem = 8,
		Infiniband = 9,
		Bond = 10,
		Vlan = 11,
		Adsl = 12,
		Bridge = 13,
		Team = 15,
		Tun = 16,
		Tunnel = 17,
		Macvlan = 18,
		Vxlan = 19,
		Veth = 20,
		Macsec = 21,
		Dummy = 22,
		Ppp = 23,
		OvsInterface = 24,
		OvsPort = 25,
		OvsBridge = 26,
		Wpan = 27,
		Lowpan = 28,
		WireGuard = 29,
		WifiP2P = 30,
		Vrf = 31,
		Loopback = 32,
		Hsr = 33,
		Ipvlan = 34,
	};
	Q_ENUM(Enum);
};

// Indicates the 802.11 mode an access point is currently in.
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

} // namespace qs::network
