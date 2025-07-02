#pragma once

#include <qdbusconnection.h>
#include <qobject.h>
#include <qstring.h>
#include <qtmetamacros.h>

#include "dbus_objectmanager_types.hpp"

class DBusObjectManagerInterface;

namespace qs::dbus {

class DBusObjectManager: public QObject {
	Q_OBJECT;

public:
	explicit DBusObjectManager(QObject* parent = nullptr);

	bool setInterface(
	    const QString& service,
	    const QString& path,
	    const QDBusConnection& connection = QDBusConnection::sessionBus()
	);

signals:
	void
	interfacesAdded(const QDBusObjectPath& objectPath, const DBusObjectManagerInterfaces& interfaces);
	void interfacesRemoved(const QDBusObjectPath& objectPath, const QStringList& interfaces);

private:
	void fetchInitialObjects();

	DBusObjectManagerInterface* mInterface = nullptr;
};

} // namespace qs::dbus