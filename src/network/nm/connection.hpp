#pragma once

#include <qcontainerfwd.h>
#include <qdbusextratypes.h>
#include <qdbusservicewatcher.h>
#include <qhash.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "nm/dbus_nm_connection.h"

namespace qs::network {

class NMConnectionAdapter: public QObject {
	Q_OBJECT;

public:
	explicit NMConnectionAdapter(const QString& path, QObject* parent = nullptr);
	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;

private:
	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMConnectionAdapter, connectionProperties);
	DBusNMConnectionProxy* proxy = nullptr;
};

} // namespace qs::network
