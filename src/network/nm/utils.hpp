
#pragma once

#include <qcontainerfwd.h>
#include <qdbusservicewatcher.h>
#include <qobject.h>
#include <qqmlintegration.h>

#include "dbus_types.hpp"
#include "enums.hpp"

namespace qs::network {

NMWirelessSecurityType::Enum securityFromConnectionSettings(const ConnectionSettingsMap& settings);

bool deviceSupportsApCiphers(
    NMWirelessCapabilities::Enum caps,
    NM80211ApSecurityFlags::Enum apFlags,
    NMWirelessSecurityType::Enum type
);

bool securityIsValid(
    NMWirelessSecurityType::Enum type,
    NMWirelessCapabilities::Enum caps,
    bool adhoc,
    NM80211ApFlags::Enum apFlags,
    NM80211ApSecurityFlags::Enum apWpa,
    NM80211ApSecurityFlags::Enum apRsn
);

NMWirelessSecurityType::Enum findBestWirelessSecurity(
    NMWirelessCapabilities::Enum caps,
    bool adHoc,
    NM80211ApFlags::Enum apFlags,
    NM80211ApSecurityFlags::Enum apWpa,
    NM80211ApSecurityFlags::Enum apRsn
);

} // namespace qs::network
