#pragma once

#include <qdbusextratypes.h>
#include <qmap.h>
#include <qstring.h>
#include <qvariant.h>

using ConnectionSettingsMap = QMap<QString, QVariantMap>;
Q_DECLARE_METATYPE(ConnectionSettingsMap);
