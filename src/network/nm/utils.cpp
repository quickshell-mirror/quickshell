#include "utils.hpp"
#include <cmath>
// We depend on non-std Linux extensions that ctime doesn't put in the global namespace
// NOLINTNEXTLINE(modernize-deprecated-headers)
#include <time.h>

#include <qcontainerfwd.h>
#include <qdatetime.h>
#include <qdbusargument.h>
#include <qdbusservicewatcher.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtypes.h>

#include "../enums.hpp"
#include "dbus_types.hpp"
#include "enums.hpp"

namespace qs::network {

WifiSecurityType::Enum securityFromSettingsMap(const NMSettingsMap& settings) {
	const QString mapName = "802-11-wireless-security";
	if (!settings.contains(mapName)) return WifiSecurityType::Unknown;
	const QVariantMap& security = settings.value(mapName);
	if (security.isEmpty()) return WifiSecurityType::Open;

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
	} else if (keyMgmt == "owe") {
		return WifiSecurityType::Owe;
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

NMSettingsMap mergeSettingsMaps(const NMSettingsMap& target, const NMSettingsMap& source) {
	NMSettingsMap result = target;
	for (auto iter = source.constBegin(); iter != source.constEnd(); ++iter) {
		result[iter.key()].insert(iter.value());
	}
	return result;
}

NMSettingsMap removeSettingsInMap(const NMSettingsMap& target, const NMSettingsMap& toRemove) {
	NMSettingsMap result = target;
	for (auto iter = toRemove.constBegin(); iter != toRemove.constEnd(); ++iter) {
		const QString& group = iter.key();
		const QVariantMap& keysToRemove = iter.value();

		if (!result.contains(group)) continue;

		for (auto jt = keysToRemove.constBegin(); jt != keysToRemove.constEnd(); ++jt) {
			result[group].remove(jt.key());
		}

		// Remove the group entirely if it's now empty
		if (result[group].isEmpty()) {
			result.remove(group);
		}
	}
	return result;
}

// Some NMSettingsMap settings remain QDBusArguments after autodemarshalling.
// Manually demarshall these for any complex signature we have registered.
void manualSettingDemarshall(NMSettingsMap& map) {
	auto demarshallValue = [](const QVariant& value) -> QVariant {
		if (value.userType() != qMetaTypeId<QDBusArgument>()) {
			return value;
		}

		auto arg = value.value<QDBusArgument>();
		auto signature = arg.currentSignature();

		if (signature == "ay") return QVariant::fromValue(qdbus_cast<QByteArray>(arg));
		if (signature == "aay") return QVariant::fromValue(qdbus_cast<QList<QByteArray>>(arg));
		if (signature == "au") return QVariant::fromValue(qdbus_cast<QList<quint32>>(arg));
		if (signature == "aau") return QVariant::fromValue(qdbus_cast<QList<QList<quint32>>>(arg));
		if (signature == "aa{sv}") return QVariant::fromValue(qdbus_cast<QList<QVariantMap>>(arg));
		if (signature == "a(ayuay)") return QVariant::fromValue(qdbus_cast<QList<NMIPv6Address>>(arg));
		if (signature == "a(ayuayu)") return QVariant::fromValue(qdbus_cast<QList<NMIPv6Route>>(arg));
		if (signature == "a{ss}") return QVariant::fromValue(qdbus_cast<QMap<QString, QString>>(arg));

		return value;
	};

	for (auto it = map.begin(); it != map.end(); ++it)
		for (auto jt = it.value().begin(); jt != it.value().end(); ++jt)
			jt.value() = demarshallValue(jt.value());
}

// Some NMSettingsMap setting types can't be expressed in QML.
// Convert these settings to their correct type or return an invalid QVariant.
QVariant settingTypeFromQml(const QString& group, const QString& key, const QVariant& value) {
	auto s = group + "." + key;

	// QString -> QByteArray
	if (s == "802-1x.ca-cert" || s == "802-1x.client-cert" || s == "802-1x.private-key"
	    || s == "802-1x.password-raw" || s == "802-1x.phase2-ca-cert"
	    || s == "802-1x.phase2-client-cert" || s == "802-1x.phase2-private-key"
	    || s == "802-11-wireless.ssid" || s == "802-3-ethernet.cloned-mac-address"
	    || s == "802-3-ethernet.mac-address")
	{
		if (value.typeId() == QMetaType::QString) {
			return value.toString().toUtf8();
		}
		if (value.typeId() == QMetaType::QByteArray) {
			return value;
		}
		return QVariant();
	}

	// QVariantList -> QList<QByteArray>
	if (s == "ipv6.dns") {
		if (value.typeId() == QMetaType::QVariantList) {
			QList<QByteArray> r;
			for (const auto& v: value.toList()) {
				if (v.typeId() == QMetaType::QString) {
					r.append(v.toString().toUtf8());
				} else {
					r.append(v.toByteArray());
				}
			}
			return QVariant::fromValue(r);
		}
		return QVariant();
	}

	// QVariantList -> QList<quint32>
	if (s == "ipv4.dns") {
		if (value.typeId() == QMetaType::QVariantList) {
			QList<quint32> r;
			for (const auto& v: value.toList()) {
				r.append(v.value<quint32>());
			}
			return QVariant::fromValue(r);
		}
		return QVariant();
	}

	// QVariantList -> QList<QList<quint32>>
	if (s == "ipv4.addresses" || s == "ipv4.routes") {
		if (value.typeId() == QMetaType::QVariantList) {
			QList<QList<quint32>> r;
			for (const auto& v: value.toList()) {
				if (v.typeId() != QMetaType::QVariantList) {
					continue;
				}
				QList<quint32> inner;
				for (const auto& u: v.toList()) {
					inner.append(u.value<quint32>());
				}
				r.append(inner);
			}
			return QVariant::fromValue(r);
		}
		return QVariant();
	}

	// QVariantList -> QList<QVariantMap>
	if (s == "ipv4.address-data" || s == "ipv4.route-data" || s == "ipv4.routing-rules"
	    || s == "ipv6.address-data" || s == "ipv6.route-data" || s == "ipv6.routing-rules")
	{
		if (value.typeId() == QMetaType::QVariantList) {
			QList<QVariantMap> r;
			for (const auto& v: value.toList()) {
				if (!v.canConvert<QVariantMap>()) {
					continue;
				}
				r.append(v.toMap());
			}
			return QVariant::fromValue(r);
		}
		return QVariant();
	}

	// QVariantList -> QList<NMIPv6Address>
	if (s == "ipv6.addresses") {
		if (value.typeId() == QMetaType::QVariantList) {
			QList<NMIPv6Address> r;
			for (const auto& v: value.toList()) {
				if (v.typeId() != QMetaType::QVariantList) {
					continue;
				}
				auto fields = v.toList();
				if (fields.size() != 3) {
					continue;
				}
				const QByteArray address = fields[0].typeId() == QMetaType::QString
				                             ? fields[0].toString().toUtf8()
				                             : fields[0].toByteArray();
				const QByteArray gateway = fields[2].typeId() == QMetaType::QString
				                             ? fields[2].toString().toUtf8()
				                             : fields[2].toByteArray();
				r.append({.address = address, .prefix = fields[1].value<quint32>(), .gateway = gateway});
			}
			return QVariant::fromValue(r);
		}
		return QVariant();
	}

	// QVariantList -> QList<NMIPv6Route>
	if (s == "ipv6.routes") {
		if (value.typeId() == QMetaType::QVariantList) {
			QList<NMIPv6Route> r;
			for (const auto& v: value.toList()) {
				if (v.typeId() != QMetaType::QVariantList) {
					continue;
				}
				auto fields = v.toList();
				if (fields.size() != 4) {
					continue;
				}
				const QByteArray destination = fields[0].typeId() == QMetaType::QString
				                                 ? fields[0].toString().toUtf8()
				                                 : fields[0].toByteArray();
				const QByteArray nexthop = fields[2].typeId() == QMetaType::QString
				                             ? fields[2].toString().toUtf8()
				                             : fields[2].toByteArray();
				r.append(
				    {.destination = destination,
				     .prefix = fields[1].value<quint32>(),
				     .nexthop = nexthop,
				     .metric = fields[3].value<quint32>()}
				);
			}
			return QVariant::fromValue(r);
		}
		return QVariant();
	}

	// QVariantList -> QStringList
	if (s == "connection.permissions" || s == "ipv4.dns-search" || s == "ipv6.dns-search"
	    || s == "802-11-wireless.seen-bssids" || s == "802-3-ethernet.mac-address-blacklist"
	    || s == "802-3-ethernet.mac-address-denylist" || s == "802-3-ethernet.s390-subchannels")
	{
		if (value.typeId() == QMetaType::QVariantList) {
			QStringList stringList;
			for (const auto& item: value.toList()) {
				stringList.append(item.toString());
			}
			return stringList;
		}
		return QVariant();
	}

	// QVariantMap -> QMap<QString, QString>
	if (s == "802-3-ethernet.s390-options") {
		if (value.canConvert<QVariantMap>()) {
			QMap<QString, QString> r;
			for (const auto& [key, val]: value.toMap().asKeyValueRange()) {
				r.insert(key, val.toString());
			}
			return QVariant::fromValue(r);
		}
		return QVariant();
	}

	// double (whole number) -> qint32
	if (value.typeId() == QMetaType::Double) {
		auto num = value.toDouble();
		if (std::isfinite(num) && num == std::trunc(num)) {
			return QVariant::fromValue(static_cast<qint32>(num));
		}
	}

	return value;
}

// Some NMSettingsMap setting types must be converted to a type that is supported by QML.
// Although QByteArrays can be represented in QML, we convert them to strings for convenience.
QVariant settingTypeToQml(const QVariant& value) {
	// QByteArray -> QString
	if (value.typeId() == QMetaType::QByteArray) {
		return QString::fromUtf8(value.toByteArray());
	}

	// QList<QByteArray> -> QVariantList
	if (value.userType() == qMetaTypeId<QList<QByteArray>>()) {
		QVariantList out;
		for (const auto& ba: value.value<QList<QByteArray>>()) {
			out.append(QString::fromUtf8(ba));
		}
		return out;
	}

	// QList<NMIPv6Address> -> QVariantList
	if (value.userType() == qMetaTypeId<QList<NMIPv6Address>>()) {
		QVariantList out;
		for (const auto& addr: value.value<QList<NMIPv6Address>>()) {
			out.append(
			    QVariant::fromValue(
			        QVariantList {
			            QString::fromUtf8(addr.address),
			            addr.prefix,
			            QString::fromUtf8(addr.gateway),
			        }
			    )
			);
		}
		return out;
	}

	// QList<NMIPv6Route> -> QVariantList
	if (value.userType() == qMetaTypeId<QList<NMIPv6Route>>()) {
		QVariantList out;
		for (const auto& route: value.value<QList<NMIPv6Route>>()) {
			out.append(
			    QVariant::fromValue(
			        QVariantList {
			            QString::fromUtf8(route.destination),
			            route.prefix,
			            QString::fromUtf8(route.nexthop),
			            route.metric,
			        }
			    )
			);
		}
		return out;
	}

	// QMap<QString, QString> -> QVariantMap
	if (value.userType() == qMetaTypeId<QMap<QString, QString>>()) {
		QVariantMap out;
		for (const auto& [key, val]: value.value<QMap<QString, QString>>().asKeyValueRange()) {
			out.insert(key, val);
		}
		return out;
	}

	return value;
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
