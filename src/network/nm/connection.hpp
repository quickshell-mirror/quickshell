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
#include "nm/dbus_nm_settings.h"

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

class NMSettingsAdapter: public QObject {
	Q_OBJECT;

public:
	explicit NMSettingsAdapter(const QString& path, QObject* parent = nullptr);

	void registerConnections();
	void registerConnection(const QString& path);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;

private slots:
	void onNewConnection(const QDBusObjectPath& path);
	void onConnectionRemoved(const QDBusObjectPath& path);

private:
	QHash<QString, NMConnectionAdapter*> mConnectionMap;

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMSettingsAdapter, settingsProperties);

	DBusNMSettingsProxy* proxy = nullptr;
};

} // namespace qs::network
