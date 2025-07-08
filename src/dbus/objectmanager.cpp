#include "objectmanager.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qdbusmetatype.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qtmetamacros.h>

#include "../core/logcat.hpp"
#include "dbus_objectmanager.h"
#include "dbus_objectmanager_types.hpp"

namespace {
QS_LOGGING_CATEGORY(logDbusObjectManager, "quickshell.dbus.objectmanager", QtWarningMsg);
}

namespace qs::dbus {

DBusObjectManager::DBusObjectManager(QObject* parent): QObject(parent) {
	qDBusRegisterMetaType<DBusObjectManagerInterfaces>();
	qDBusRegisterMetaType<DBusObjectManagerObjects>();
}

bool DBusObjectManager::setInterface(
    const QString& service,
    const QString& path,
    const QDBusConnection& connection
) {
	delete this->mInterface;
	this->mInterface = new DBusObjectManagerInterface(service, path, connection, this);

	if (!this->mInterface->isValid()) {
		qCWarning(logDbusObjectManager) << "Failed to create DBusObjectManagerInterface for" << service
		                                << path << ":" << this->mInterface->lastError();
		delete this->mInterface;
		this->mInterface = nullptr;
		return false;
	}

	QObject::connect(
	    this->mInterface,
	    &DBusObjectManagerInterface::InterfacesAdded,
	    this,
	    &DBusObjectManager::interfacesAdded
	);

	QObject::connect(
	    this->mInterface,
	    &DBusObjectManagerInterface::InterfacesRemoved,
	    this,
	    &DBusObjectManager::interfacesRemoved
	);

	this->fetchInitialObjects();
	return true;
}

void DBusObjectManager::fetchInitialObjects() {
	if (!this->mInterface) return;

	auto reply = this->mInterface->GetManagedObjects();
	auto* watcher = new QDBusPendingCallWatcher(reply, this);

	QObject::connect(
	    watcher,
	    &QDBusPendingCallWatcher::finished,
	    this,
	    [this](QDBusPendingCallWatcher* watcher) {
		    const QDBusPendingReply<DBusObjectManagerObjects> reply = *watcher;
		    watcher->deleteLater();

		    if (reply.isError()) {
			    qCWarning(logDbusObjectManager) << "Failed to get managed objects:" << reply.error();
			    return;
		    }

		    for (const auto& [path, interfaces]: reply.value().asKeyValueRange()) {
			    emit this->interfacesAdded(path, interfaces);
		    }
	    }
	);
}

} // namespace qs::dbus
