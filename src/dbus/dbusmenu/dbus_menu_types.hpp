#pragma once

#include <qcontainerfwd.h>
#include <qdbusargument.h>
#include <qdebug.h>
#include <qlist.h>
#include <qtypes.h>

struct DBusMenuLayout {
	qint32 id = 0;
	QVariantMap properties;
	QList<DBusMenuLayout> children;
};

using DBusMenuIdList = QList<qint32>;

struct DBusMenuItemProperties {
	qint32 id = 0;
	QVariantMap properties;
};

using DBusMenuItemPropertiesList = QList<DBusMenuItemProperties>;

struct DBusMenuItemPropertyNames {
	qint32 id = 0;
	QStringList properties;
};

using DBusMenuItemPropertyNamesList = QList<DBusMenuItemPropertyNames>;

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusMenuLayout& layout);
const QDBusArgument& operator<<(QDBusArgument& argument, const DBusMenuLayout& layout);
const QDBusArgument& operator>>(const QDBusArgument& argument, DBusMenuItemProperties& item);
const QDBusArgument& operator<<(QDBusArgument& argument, const DBusMenuItemProperties& item);
const QDBusArgument& operator>>(const QDBusArgument& argument, DBusMenuItemPropertyNames& names);
const QDBusArgument& operator<<(QDBusArgument& argument, const DBusMenuItemPropertyNames& names);

QDebug operator<<(QDebug debug, const DBusMenuLayout& layout);
QDebug operator<<(QDebug debug, const DBusMenuItemProperties& item);
QDebug operator<<(QDebug debug, const DBusMenuItemPropertyNames& names);

Q_DECLARE_METATYPE(DBusMenuLayout);
Q_DECLARE_METATYPE(DBusMenuIdList);
Q_DECLARE_METATYPE(DBusMenuItemProperties);
Q_DECLARE_METATYPE(DBusMenuItemPropertiesList);
Q_DECLARE_METATYPE(DBusMenuItemPropertyNames);
Q_DECLARE_METATYPE(DBusMenuItemPropertyNamesList);
