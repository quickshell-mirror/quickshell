#include "dbus_menu_types.hpp"

#include <qdbusargument.h>
#include <qdbusextratypes.h>
#include <qdebug.h>
#include <qmetatype.h>
#include <qvariant.h>

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusMenuLayout& layout) {
	layout.children.clear();

	argument.beginStructure();
	argument >> layout.id;
	argument >> layout.properties;

	argument.beginArray();
	while (!argument.atEnd()) {
		auto childArgument = qdbus_cast<QDBusVariant>(argument).variant().value<QDBusArgument>();
		auto child = qdbus_cast<DBusMenuLayout>(childArgument);
		layout.children.append(child);
	}
	argument.endArray();

	argument.endStructure();
	return argument;
}

const QDBusArgument& operator<<(QDBusArgument& argument, const DBusMenuLayout& layout) {
	argument.beginStructure();
	argument << layout.id;
	argument << layout.properties;

	argument.beginArray(qMetaTypeId<QDBusVariant>());
	for (const auto& child: layout.children) {
		argument << QDBusVariant(QVariant::fromValue(child));
	}
	argument.endArray();

	argument.endStructure();
	return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusMenuItemProperties& item) {
	argument.beginStructure();
	argument >> item.id;
	argument >> item.properties;
	argument.endStructure();
	return argument;
}

const QDBusArgument& operator<<(QDBusArgument& argument, const DBusMenuItemProperties& item) {
	argument.beginStructure();
	argument << item.id;
	argument << item.properties;
	argument.endStructure();
	return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusMenuItemPropertyNames& names) {
	argument.beginStructure();
	argument >> names.id;
	argument >> names.properties;
	argument.endStructure();
	return argument;
}

const QDBusArgument& operator<<(QDBusArgument& argument, const DBusMenuItemPropertyNames& names) {
	argument.beginStructure();
	argument << names.id;
	argument << names.properties;
	argument.endStructure();
	return argument;
}

QDebug operator<<(QDebug debug, const DBusMenuLayout& layout) {
	debug.nospace() << "DBusMenuLayout(id=" << layout.id << ", properties=" << layout.properties
	                << ", children=" << layout.children << ")";

	return debug;
}

QDebug operator<<(QDebug debug, const DBusMenuItemProperties& item) {
	debug.nospace() << "DBusMenuItemProperties(id=" << item.id << ", properties=" << item.properties
	                << ")";
	return debug;
}

QDebug operator<<(QDebug debug, const DBusMenuItemPropertyNames& names) {
	debug.nospace() << "DBusMenuItemPropertyNames(id=" << names.id
	                << ", properties=" << names.properties << ")";
	return debug;
}
