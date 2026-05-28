#include "dbus_types.hpp"

#include <qbytearray.h>
#include <qcontainerfwd.h>
#include <qdbusargument.h>
#include <qmap.h>
#include <qmetatype.h>
#include <qstring.h>

namespace qs::network {

const QDBusArgument& operator>>(const QDBusArgument& argument, NMSettingsMap& map) {
	argument.beginMap();
	while (!argument.atEnd()) {
		argument.beginMapEntry();
		QString groupName;
		argument >> groupName;

		QVariantMap group;
		argument >> group;

		map.insert(groupName, group);
		argument.endMapEntry();
	}
	argument.endMap();
	return argument;
}

const QDBusArgument& operator<<(QDBusArgument& argument, const NMSettingsMap& map) {
	argument.beginMap(qMetaTypeId<QString>(), qMetaTypeId<QVariantMap>());
	for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
		argument.beginMapEntry();
		argument << it.key();
		argument << it.value();
		argument.endMapEntry();
	}
	argument.endMap();
	return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, NMIPv6Address& addr) {
	argument.beginStructure();
	argument >> addr.address >> addr.prefix >> addr.gateway;
	argument.endStructure();
	return argument;
}

const QDBusArgument& operator<<(QDBusArgument& argument, const NMIPv6Address& addr) {
	argument.beginStructure();
	argument << addr.address << addr.prefix << addr.gateway;
	argument.endStructure();
	return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, NMIPv6Route& route) {
	argument.beginStructure();
	argument >> route.destination >> route.prefix >> route.nexthop >> route.metric;
	argument.endStructure();
	return argument;
}

const QDBusArgument& operator<<(QDBusArgument& argument, const NMIPv6Route& route) {
	argument.beginStructure();
	argument << route.destination << route.prefix << route.nexthop << route.metric;
	argument.endStructure();
	return argument;
}

} // namespace qs::network
