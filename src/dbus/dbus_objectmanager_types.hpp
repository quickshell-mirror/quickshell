#pragma once

#include <qdbusextratypes.h>
#include <qhash.h>
#include <qmap.h>
#include <qstring.h>
#include <qvariant.h>

using DBusObjectManagerInterfaces = QHash<QString, QVariantMap>;
using DBusObjectManagerObjects = QHash<QDBusObjectPath, DBusObjectManagerInterfaces>;
