#include "connection.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "nm/dbus_nm_connection.h"

using namespace qs::dbus;

namespace qs::network {

namespace {
Q_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

NMConnectionAdapter::NMConnectionAdapter(const QString& path, QObject* parent): QObject(parent) {
	this->proxy = new DBusNMConnectionProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->proxy->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create DBus interface for connection at" << path;
		return;
	}

	this->connectionProperties.setInterface(this->proxy);
	this->connectionProperties.updateAllViaGetAll();
}

bool NMConnectionAdapter::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMConnectionAdapter::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMConnectionAdapter::path() const { return this->proxy ? this->proxy->path() : QString(); }

NMSettingsAdapter::NMSettingsAdapter(const QString& path, QObject* parent): QObject(parent) {
	this->proxy = new DBusNMSettingsProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->proxy->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create DBus interface for connection at" << path;
		return;
	}

	QObject::connect(
	    this->proxy,
	    &DBusNMSettingsProxy::NewConnection,
	    this,
	    &NMSettingsAdapter::onNewConnection
	);

	QObject::connect(
	    this->proxy,
	    &DBusNMSettingsProxy::ConnectionRemoved,
	    this,
	    &NMSettingsAdapter::onConnectionRemoved
	);

	this->settingsProperties.setInterface(this->proxy);
	this->settingsProperties.updateAllViaGetAll();

	this->registerConnections();
}

void NMSettingsAdapter::onNewConnection(const QDBusObjectPath& path) {
	this->registerConnection(path.path());
}

void NMSettingsAdapter::onConnectionRemoved(const QDBusObjectPath& path) {
	auto* connection = this->mConnectionMap.take(path.path());	

	if (!connection) {
		qCDebug(logNetworkManager) << "NetworkManager backend sent removal signal for" << path.path()
		                           << "which is not registered.";
		return;
	}

	delete connection;
}

void NMSettingsAdapter::registerConnections() {
	auto pending = this->proxy->ListConnections();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<QList<QDBusObjectPath>> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager) << "Failed to get connections: " << reply.error().message();
		} else {
			for (const QDBusObjectPath& devicePath: reply.value()) {
				this->registerConnection(devicePath.path());
			}
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void NMSettingsAdapter::registerConnection(const QString& path) {
	auto* connection = new NMConnectionAdapter(path, this);

	if (!connection->isValid()) {
		qCWarning(logNetworkManager) << "Ignoring invalid registration of" << path;
		delete connection;
		return;
	}

	this->mConnectionMap.insert(path, connection);
	qCDebug(logNetworkManager) << "Registered connection" << connection;
}

bool NMSettingsAdapter::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMSettingsAdapter::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMSettingsAdapter::path() const { return this->proxy ? this->proxy->path() : QString(); }

} // namespace qs::network
