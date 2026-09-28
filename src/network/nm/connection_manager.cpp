#include "connection_manager.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qdbusextratypes.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qhash.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include "../../core/logcat.hpp"
#include "../../dbus/properties.hpp"
#include "connection.hpp"
#include "dbus_nm_settings.h"

namespace qs::network {

namespace {
QS_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
const QStringList SUPPORTED_CONNECTION_TYPES {"802-11-wireless", "802-3-ethernet"};
} // namespace

NMConnectionManager::NMConnectionManager(QObject* parent): QObject(parent) {
	this->proxy = new DBusNMSettingsProxy(
	    "org.freedesktop.NetworkManager",
	    "/org/freedesktop/NetworkManager/Settings",
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->proxy->isValid()) {
		qCWarning(
		    logNetworkManager
		) << "Cannot create DBus interface for /org/freedesktop/NetworkManager/Settings";
	}

	// clang-format off
	QObject::connect(this->proxy, &DBusNMSettingsProxy::NewConnection, this, &NMConnectionManager::onNewConnectionPath);
	QObject::connect(this->proxy, &DBusNMSettingsProxy::ConnectionRemoved, this, &NMConnectionManager::onConnectionPathRemoved);
	// clang-format on

	this->connectionManagerProperties.setInterface(this->proxy);
}

void NMConnectionManager::onServiceRegistered() {
	this->connectionManagerProperties.updateAllViaGetAll();
	this->registerConnections();
}

void NMConnectionManager::onServiceUnregistered() {
	const auto connections = this->mConnections;
	const auto loadedConnections = this->mLoadedConnections;
	this->mConnections.clear();
	this->mLoadedConnections.clear();

	for (auto* conn: connections) {
		qCDebug(logNetworkManager) << "Connection removed:" << conn->path();
		if (loadedConnections.contains(conn)) {
			emit conn->unregistered();
			emit this->knownNetworkRemoved(conn->frontend());
		}
		delete conn;
	}
}

void NMConnectionManager::registerConnections() {
	auto pending = this->proxy->ListConnections();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<QList<QDBusObjectPath>> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager) << "Failed to get connections: " << reply.error().message();
		} else {
			for (const QDBusObjectPath& connectionPath: reply.value()) {
				this->registerConnection(connectionPath.path());
			}
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void NMConnectionManager::registerConnection(const QString& path) {
	if (this->mConnections.contains(path)) {
		qCDebug(logNetworkManager) << "Skipping duplicate registration of connection" << path;
		return;
	}

	auto* conn = new NMConnection(path, this);
	if (!conn->isValid()) {
		qCWarning(logNetworkManager) << "Ignoring invalid registration of " << path;
		delete conn;
		return;
	}

	this->mConnections[path] = conn;
	QObject::connect(conn, &NMConnection::loaded, this, [this, conn, path]() {
		const QString type = conn->settings().value("connection").value("type").toString();
		if (!SUPPORTED_CONNECTION_TYPES.contains(type)) {
			qCDebug(logNetworkManager) << "Ignoring registration of unsupported connection:" << path;
			this->mConnections.remove(path);
			conn->deleteLater();
			return;
		}

		qCDebug(logNetworkManager) << "Connection added:" << path;
		this->mLoadedConnections.append(conn);
		emit this->connectionLoaded(conn);
		emit this->knownNetworkAdded(conn->frontend());
	});
}

void NMConnectionManager::onNewConnectionPath(const QDBusObjectPath& path) {
	this->registerConnection(path.path());
}

void NMConnectionManager::onConnectionPathRemoved(const QDBusObjectPath& path) {
	auto iter = this->mConnections.find(path.path());
	if (iter == this->mConnections.end()) {
		qCWarning(logNetworkManager) << "Sent removal signal for" << path.path()
		                             << "which is not registered.";
	} else {
		auto* conn = iter.value();
		this->mConnections.erase(iter);
		qCDebug(logNetworkManager) << "Connection removed:" << path.path();
		if (this->mLoadedConnections.removeOne(conn)) {
			emit conn->unregistered();
			emit this->knownNetworkRemoved(conn->frontend());
		}
		delete conn;
	}
}

} // namespace qs::network
