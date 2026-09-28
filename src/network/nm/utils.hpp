#pragma once

#include <qcontainerfwd.h>
#include <qdbusservicewatcher.h>
#include <qobject.h>

#include "../enums.hpp"
#include "dbus_types.hpp"
#include "enums.hpp"

namespace qs::network {

WifiSecurityType::Enum securityFromSettings(const NMSettings& settings);

bool deviceSupportsApCiphers(
    NMWirelessCapabilities::Enum caps,
    NM80211ApSecurityFlags::Enum apFlags,
    WifiSecurityType::Enum type
);

// In sync with NetworkManager/libnm-core/nm-utils.c:nm_utils_security_valid()
// Given a set of device capabilities, and a desired security type to check
// against, determines whether the combination of device, desired security type,
// and AP capabilities intersect.
bool securityIsValid(
    WifiSecurityType::Enum type,
    NMWirelessCapabilities::Enum caps,
    bool adhoc,
    NM80211ApFlags::Enum apFlags,
    NM80211ApSecurityFlags::Enum apWpa,
    NM80211ApSecurityFlags::Enum apRsn
);

WifiSecurityType::Enum findBestWirelessSecurity(
    NMWirelessCapabilities::Enum caps,
    bool adHoc,
    NM80211ApFlags::Enum apFlags,
    NM80211ApSecurityFlags::Enum apWpa,
    NM80211ApSecurityFlags::Enum apRsn
);

NMSettings mergeSettings(const NMSettings& target, const NMSettings& source);

NMSettings removeSettings(const NMSettings& target, const NMSettings& toRemove);

void manualSettingDemarshall(NMSettings& map);

QVariant settingTypeFromQml(const QString& group, const QString& key, const QVariant& value);

QVariant settingTypeToQml(const QVariant& value);

QVariantMap settingsToQml(const NMSettings& settings);

QDateTime clockBootTimeToDateTime(qint64 clockBootTime);

} // namespace qs::network
