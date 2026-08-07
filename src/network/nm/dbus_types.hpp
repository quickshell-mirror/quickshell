#pragma once

#include <qbytearray.h>
#include <qdbusargument.h>
#include <qmap.h>
#include <qstring.h>
#include <qtypes.h>
#include <qvariant.h>

namespace qs::network {

using NMSettingsMap = QMap<QString, QVariantMap>;

const QDBusArgument& operator>>(const QDBusArgument& argument, NMSettingsMap& map);
const QDBusArgument& operator<<(QDBusArgument& argument, const NMSettingsMap& map);

struct NMIPv6Address {
	QByteArray address;
	quint32 prefix = 0;
	QByteArray gateway;
};

const QDBusArgument& operator>>(const QDBusArgument& argument, qs::network::NMIPv6Address& addr);
const QDBusArgument& operator<<(QDBusArgument& argument, const qs::network::NMIPv6Address& addr);

struct NMIPv6Route {
	QByteArray destination;
	quint32 prefix = 0;
	QByteArray nexthop;
	quint32 metric = 0;
};

const QDBusArgument& operator>>(const QDBusArgument& argument, qs::network::NMIPv6Route& route);
const QDBusArgument& operator<<(QDBusArgument& argument, const qs::network::NMIPv6Route& route);

} // namespace qs::network

Q_DECLARE_METATYPE(qs::network::NMSettingsMap);
Q_DECLARE_METATYPE(qs::network::NMIPv6Address);
Q_DECLARE_METATYPE(qs::network::NMIPv6Route);
