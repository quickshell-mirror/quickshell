#include "utils.hpp"

// We depend on non-std Linux extensions that ctime doesn't put in the global namespace
// NOLINTNEXTLINE(modernize-deprecated-headers)
#include <time.h>

#include <qcontainerfwd.h>
#include <qdatetime.h>
#include <qdbusservicewatcher.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtypes.h>

#include "../wifi.hpp"
#include "enums.hpp"
#include "types.hpp"

namespace qs::network {

WifiSecurityType::Enum securityFromConnectionSettings(const ConnectionSettingsMap& settings) {
	const QVariantMap& security = settings.value("802-11-wireless-security");
	if (security.isEmpty()) {
		return WifiSecurityType::Open;
	};

	const QString keyMgmt = security["key-mgmt"].toString();
	const QString authAlg = security["auth-alg"].toString();
	const QList<QVariant> proto = security["proto"].toList();

	if (keyMgmt == "none") {
		return WifiSecurityType::StaticWep;
	} else if (keyMgmt == "ieee8021x") {
		if (authAlg == "leap") {
			return WifiSecurityType::Leap;
		} else {
			return WifiSecurityType::DynamicWep;
		}
	} else if (keyMgmt == "wpa-psk") {
		if (proto.contains("wpa") && proto.contains("rsn")) return WifiSecurityType::WpaPsk;
		return WifiSecurityType::Wpa2Psk;
	} else if (keyMgmt == "wpa-eap") {
		if (proto.contains("wpa") && proto.contains("rsn")) return WifiSecurityType::WpaEap;
		return WifiSecurityType::Wpa2Eap;
	} else if (keyMgmt == "sae") {
		return WifiSecurityType::Sae;
	} else if (keyMgmt == "wpa-eap-suite-b-192") {
		return WifiSecurityType::Wpa3SuiteB192;
	}
	return WifiSecurityType::Open;
}

bool deviceSupportsApCiphers(
    NMWirelessCapabilities::Enum caps,
    NM80211ApSecurityFlags::Enum apFlags,
    WifiSecurityType::Enum type
) {
	bool havePair = false;
	bool haveGroup = false;
	// Device needs to support at least one pairwise and one group cipher

	if (type == WifiSecurityType::StaticWep) {
		// Static WEP only uses group ciphers
		havePair = true;
	} else {
		if (caps & NMWirelessCapabilities::CipherWep40 && apFlags & NM80211ApSecurityFlags::PairWep40) {
			havePair = true;
		}
		if (caps & NMWirelessCapabilities::CipherWep104 && apFlags & NM80211ApSecurityFlags::PairWep104)
		{
			havePair = true;
		}
		if (caps & NMWirelessCapabilities::CipherTkip && apFlags & NM80211ApSecurityFlags::PairTkip) {
			havePair = true;
		}
		if (caps & NMWirelessCapabilities::CipherCcmp && apFlags & NM80211ApSecurityFlags::PairCcmp) {
			havePair = true;
		}
	}

	if (caps & NMWirelessCapabilities::CipherWep40 && apFlags & NM80211ApSecurityFlags::GroupWep40) {
		haveGroup = true;
	}
	if (caps & NMWirelessCapabilities::CipherWep104 && apFlags & NM80211ApSecurityFlags::GroupWep104)
	{
		haveGroup = true;
	}
	if (type != WifiSecurityType::StaticWep) {
		if (caps & NMWirelessCapabilities::CipherTkip && apFlags & NM80211ApSecurityFlags::GroupTkip) {
			haveGroup = true;
		}
		if (caps & NMWirelessCapabilities::CipherCcmp && apFlags & NM80211ApSecurityFlags::GroupCcmp) {
			haveGroup = true;
		}
	}

	return (havePair && haveGroup);
}

bool securityIsValid(
    WifiSecurityType::Enum type,
    NMWirelessCapabilities::Enum caps,
    bool adhoc,
    NM80211ApFlags::Enum apFlags,
    NM80211ApSecurityFlags::Enum apWpa,
    NM80211ApSecurityFlags::Enum apRsn
) {
	switch (type) {
	case WifiSecurityType::Open:
		if (apFlags & NM80211ApFlags::Privacy) return false;
		if (apWpa || apRsn) return false;
		break;
	case WifiSecurityType::Leap:
		if (adhoc) return false;
	case WifiSecurityType::StaticWep:
		if (!(apFlags & NM80211ApFlags::Privacy)) return false;
		if (apWpa || apRsn) {
			if (!deviceSupportsApCiphers(caps, apWpa, WifiSecurityType::StaticWep)) {
				if (!deviceSupportsApCiphers(caps, apRsn, WifiSecurityType::StaticWep)) return false;
			}
		}
		break;
	case WifiSecurityType::DynamicWep:
		if (adhoc) return false;
		if (apRsn || !(apFlags & NM80211ApFlags::Privacy)) return false;
		if (apWpa) {
			if (!(apWpa & NM80211ApSecurityFlags::KeyMgmt8021x)) return false;
			if (!deviceSupportsApCiphers(caps, apWpa, WifiSecurityType::DynamicWep)) return false;
		}
		break;
	case WifiSecurityType::WpaPsk:
		if (adhoc) return false;
		if (!(caps & NMWirelessCapabilities::Wpa)) return false;
		if (apWpa & NM80211ApSecurityFlags::KeyMgmtPsk) {
			if (apWpa & NM80211ApSecurityFlags::PairTkip && caps & NMWirelessCapabilities::CipherTkip) {
				return true;
			}
			if (apWpa & NM80211ApSecurityFlags::PairCcmp && caps & NMWirelessCapabilities::CipherCcmp) {
				return true;
			}
		}
		return false;
	case WifiSecurityType::Wpa2Psk:
		if (!(caps & NMWirelessCapabilities::Rsn)) return false;
		if (adhoc) {
			if (!(caps & NMWirelessCapabilities::IbssRsn)) return false;
			if (apRsn & NM80211ApSecurityFlags::PairCcmp && caps & NMWirelessCapabilities::CipherCcmp) {
				return true;
			}
		} else {
			if (apRsn & NM80211ApSecurityFlags::KeyMgmtPsk) {
				if (apRsn & NM80211ApSecurityFlags::PairTkip && caps & NMWirelessCapabilities::CipherTkip) {
					return true;
				}
				if (apRsn & NM80211ApSecurityFlags::PairCcmp && caps & NMWirelessCapabilities::CipherCcmp) {
					return true;
				}
			}
		}
		return false;
	case WifiSecurityType::WpaEap:
		if (adhoc) return false;
		if (!(caps & NMWirelessCapabilities::Wpa)) return false;
		if (!(apWpa & NM80211ApSecurityFlags::KeyMgmt8021x)) return false;
		if (!deviceSupportsApCiphers(caps, apWpa, WifiSecurityType::WpaEap)) return false;
		break;
	case WifiSecurityType::Wpa2Eap:
		if (adhoc) return false;
		if (!(caps & NMWirelessCapabilities::Rsn)) return false;
		if (!(apRsn & NM80211ApSecurityFlags::KeyMgmt8021x)) return false;
		if (!deviceSupportsApCiphers(caps, apRsn, WifiSecurityType::Wpa2Eap)) return false;
		break;
	case WifiSecurityType::Sae:
		if (!(caps & NMWirelessCapabilities::Rsn)) return false;
		if (adhoc) {
			if (!(caps & NMWirelessCapabilities::IbssRsn)) return false;
			if (apRsn & NM80211ApSecurityFlags::PairCcmp && caps & NMWirelessCapabilities::CipherCcmp) {
				return true;
			}
		} else {
			if (apRsn & NM80211ApSecurityFlags::KeyMgmtSae) {
				if (apRsn & NM80211ApSecurityFlags::PairTkip && caps & NMWirelessCapabilities::CipherTkip) {
					return true;
				}
				if (apRsn & NM80211ApSecurityFlags::PairCcmp && caps & NMWirelessCapabilities::CipherCcmp) {
					return true;
				}
			}
		}
		return false;
	case WifiSecurityType::Owe:
		if (adhoc) return false;
		if (!(caps & NMWirelessCapabilities::Rsn)) return false;
		if (!(apRsn & NM80211ApSecurityFlags::KeyMgmtOwe)
		    && !(apRsn & NM80211ApSecurityFlags::KeyMgmtOweTm))
		{
			return false;
		}
		break;
	case WifiSecurityType::Wpa3SuiteB192:
		if (adhoc) return false;
		if (!(caps & NMWirelessCapabilities::Rsn)) return false;
		if (!(apRsn & NM80211ApSecurityFlags::KeyMgmtEapSuiteB192)) return false;
		break;
	default: return false;
	}
	return true;
}

WifiSecurityType::Enum findBestWirelessSecurity(
    NMWirelessCapabilities::Enum caps,
    bool adHoc,
    NM80211ApFlags::Enum apFlags,
    NM80211ApSecurityFlags::Enum apWpa,
    NM80211ApSecurityFlags::Enum apRsn
) {
	// Loop through security types from most to least secure since the enum
	// values are sequential and in priority order (0-10, excluding Unknown=11)
	for (int i = WifiSecurityType::Wpa3SuiteB192; i <= WifiSecurityType::Open; ++i) {
		auto type = static_cast<WifiSecurityType::Enum>(i);
		if (securityIsValid(type, caps, adHoc, apFlags, apWpa, apRsn)) {
			return type;
		}
	}
	return WifiSecurityType::Unknown;
}

ConnectionSettingsMap
mergeSettingsMaps(const ConnectionSettingsMap& target, const ConnectionSettingsMap& source) {
	ConnectionSettingsMap result = target;
	for (auto iter = source.constBegin(); iter != source.constEnd(); ++iter) {
		result[iter.key()].insert(iter.value());
	}
	return result;
}

// NOLINTBEGIN
QDateTime clockBootTimeToDateTime(qint64 clockBootTime) {
	clockid_t clkId = CLOCK_BOOTTIME;
	struct timespec tp {};

	const QDateTime now = QDateTime::currentDateTime();
	int r = clock_gettime(clkId, &tp);
	if (r == -1 && errno == EINVAL) {
		clkId = CLOCK_MONOTONIC;
		r = clock_gettime(clkId, &tp);
	}

	// Convert to milliseconds
	const qint64 nowInMs = tp.tv_sec * 1000 + tp.tv_nsec / 1000000;

	// Return a QDateTime of the millisecond diff
	const qint64 offset = clockBootTime - nowInMs;
	return QDateTime::fromMSecsSinceEpoch(now.toMSecsSinceEpoch() + offset);
}
// NOLINTEND

} // namespace qs::network
