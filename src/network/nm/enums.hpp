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

} // namespace qs::network
