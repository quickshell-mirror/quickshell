#include "utils.hpp"

#include <qcontainerfwd.h>
#include <qdbusservicewatcher.h>
#include <qobject.h>
#include <qqmlintegration.h>

#include "dbus_types.hpp"
#include "enums.hpp"

namespace qs::network {

NMWirelessSecurityType::Enum securityFromConnectionSettings(const ConnectionSettingsMap& settings) {
	const QVariantMap& security = settings.value("802-11-wireless-security");
	if (security.isEmpty()) {
		return NMWirelessSecurityType::Unknown;
	};

	const QString keyMgmt = security["key-mgmt"].toString();
	const QString authAlg = security["auth-alg"].toString();
	const QList<QVariant> proto = security["proto"].toList();

	if (keyMgmt == "none") {
		return NMWirelessSecurityType::StaticWep;
	} else if (keyMgmt == "ieee8021x") {
		if (authAlg == "leap") {
			return NMWirelessSecurityType::Leap;
		} else {
			return NMWirelessSecurityType::DynamicWep;
		}
	} else if (keyMgmt == "wpa-psk") {
		if (proto.contains("wpa") && proto.contains("rsn")) return NMWirelessSecurityType::WpaPsk;
		return NMWirelessSecurityType::Wpa2Psk;
	} else if (keyMgmt == "wpa-eap") {
		if (proto.contains("wpa") && proto.contains("rsn")) return NMWirelessSecurityType::WpaEap;
		return NMWirelessSecurityType::Wpa2Eap;
	} else if (keyMgmt == "sae") {
		return NMWirelessSecurityType::Sae;
	} else if (keyMgmt == "wpa-eap-suite-b-192") {
		return NMWirelessSecurityType::Wpa3SuiteB192;
	}
	return NMWirelessSecurityType::None;
}

bool deviceSupportsApCiphers(
    NMWirelessCapabilities::Enum caps,
    NM80211ApSecurityFlags::Enum apFlags,
    NMWirelessSecurityType::Enum type
) {
	bool havePair = false;
	bool haveGroup = false;
	// Device needs to support at least one pairwise and one group cipher

	if (type == NMWirelessSecurityType::StaticWep) {
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
	if (type == NMWirelessSecurityType::StaticWep) {
		if (caps & NMWirelessCapabilities::CipherTkip && apFlags & NM80211ApSecurityFlags::GroupTkip) {
			haveGroup = true;
		}
		if (caps & NMWirelessCapabilities::CipherCcmp && apFlags & NM80211ApSecurityFlags::GroupCcmp) {
			haveGroup = true;
		}
	}

	return (havePair && haveGroup);
}

// In sync with NetworkManager/libnm-core/nm-utils.c:nm_utils_security_valid()
// Given a set of device capabilities, and a desired security type to check
// against, determines whether the combination of device, desired security type,
// and AP capabilities intersect.
bool securityIsValid(
    NMWirelessSecurityType::Enum type,
    NMWirelessCapabilities::Enum caps,
    bool adhoc,
    NM80211ApFlags::Enum apFlags,
    NM80211ApSecurityFlags::Enum apWpa,
    NM80211ApSecurityFlags::Enum apRsn
) {
	switch (type) {
	case NMWirelessSecurityType::None:
		if (apFlags & NM80211ApFlags::Privacy) return false;
		if (apWpa || apRsn) return false;
		break;
	case NMWirelessSecurityType::Leap:
		if (adhoc) return false;
	case NMWirelessSecurityType::StaticWep:
		if (!(apFlags & NM80211ApFlags::Privacy)) return false;
		if (apWpa || apRsn) {
			if (!deviceSupportsApCiphers(caps, apWpa, NMWirelessSecurityType::StaticWep)) {
				if (!deviceSupportsApCiphers(caps, apRsn, NMWirelessSecurityType::StaticWep)) return false;
			}
		}
		break;
	case NMWirelessSecurityType::DynamicWep:
		if (adhoc) return false;
		if (apRsn || !(apFlags & NM80211ApFlags::Privacy)) return false;
		if (apWpa) {
			if (!(apWpa & NM80211ApSecurityFlags::KeyMgmt8021x)) return false;
			if (!deviceSupportsApCiphers(caps, apWpa, NMWirelessSecurityType::DynamicWep)) return false;
		}
		break;
	case NMWirelessSecurityType::WpaPsk:
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
	case NMWirelessSecurityType::Wpa2Psk:
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
	case NMWirelessSecurityType::WpaEap:
		if (adhoc) return false;
		if (!(caps & NMWirelessCapabilities::Wpa)) return false;
		if (!(apWpa & NM80211ApSecurityFlags::KeyMgmt8021x)) return false;
		if (!deviceSupportsApCiphers(caps, apWpa, NMWirelessSecurityType::WpaEap)) return false;
		break;
	case NMWirelessSecurityType::Wpa2Eap:
		if (adhoc) return false;
		if (!(caps & NMWirelessCapabilities::Rsn)) return false;
		if (!(apRsn & NM80211ApSecurityFlags::KeyMgmt8021x)) return false;
		if (!deviceSupportsApCiphers(caps, apRsn, NMWirelessSecurityType::Wpa2Eap)) return false;
		break;
	case NMWirelessSecurityType::Sae:
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
	case NMWirelessSecurityType::Owe:
		if (adhoc) return false;
		if (!(caps & NMWirelessCapabilities::Rsn)) return false;
		if (!(apRsn & NM80211ApSecurityFlags::KeyMgmtOwe)
		    && !(apRsn & NM80211ApSecurityFlags::KeyMgmtOweTm))
		{
			return false;
		}
		break;
	case NMWirelessSecurityType::Wpa3SuiteB192:
		if (adhoc) return false;
		if (!(caps & NMWirelessCapabilities::Rsn)) return false;
		if (!(apRsn & NM80211ApSecurityFlags::KeyMgmtEapSuiteB192)) return false;
		break;
	default: return false;
	}
	return true;
}

NMWirelessSecurityType::Enum findBestWirelessSecurity(
    NMWirelessCapabilities::Enum caps,
    bool adHoc,
    NM80211ApFlags::Enum apFlags,
    NM80211ApSecurityFlags::Enum apWpa,
    NM80211ApSecurityFlags::Enum apRsn
) {
	// Loop through security types from most to least secure since the enum
	// values are sequential and in priority order (0-10, excluding Unknown=11)
	for (int i = NMWirelessSecurityType::Wpa3SuiteB192; i <= NMWirelessSecurityType::None; ++i) {
		auto type = static_cast<NMWirelessSecurityType::Enum>(i);
		if (securityIsValid(type, caps, adHoc, apFlags, apWpa, apRsn)) {
			return type;
		}
	}
	return NMWirelessSecurityType::Unknown;
}

} // namespace qs::network
