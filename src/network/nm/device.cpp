#include "device.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"

namespace qs::dbus {

DBusResult<qs::network::NMDeviceType::Enum>
DBusDataTransform<qs::network::NMDeviceType::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<qs::network::NMDeviceType::Enum>(wire));
}

DBusResult<qs::network::NMDeviceState::Enum>
DBusDataTransform<qs::network::NMDeviceState::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<qs::network::NMDeviceState::Enum>(wire));
}

} // namespace qs::dbus

namespace qs::network {
using namespace qs::dbus;

namespace {
Q_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

NMDeviceAdapter::NMDeviceAdapter(const QString& path, QObject* parent): QObject(parent) {
	this->proxy = new DBusNMDeviceProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->proxy->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create DBus interface for device at" << path;
		return;
	}

	// clang-format off
	QObject::connect(this, &NMDeviceAdapter::availableConnectionsChanged, this, &NMDeviceAdapter::onAvailableConnectionsChanged);
	QObject::connect(this, &NMDeviceAdapter::activeConnectionChanged, this, &NMDeviceAdapter::onActiveConnectionChanged);
	QObject::connect(&this->deviceProperties, &DBusPropertyGroup::getAllFinished, this, [this]() { emit this->ready(); }, Qt::SingleShotConnection);
	// clang-format on

	this->deviceProperties.setInterface(this->proxy);
	this->deviceProperties.updateAllViaGetAll();
}

void NMDeviceAdapter::onActiveConnectionChanged(const QDBusObjectPath& path) {
	QString stringPath = path.path();
	if (this->mActiveConnection) {
		QObject::disconnect(this->mActiveConnection, nullptr, this, nullptr);
		emit this->activeConnectionRemoved(this->mActiveConnection);
		delete this->mActiveConnection;
		this->mActiveConnection = nullptr;
	}

	if (stringPath != "/") {
		auto* active = new NMActiveConnectionAdapter(stringPath, this);
		this->mActiveConnection = active;
		qCDebug(logNetworkManager) << "Registered active connection" << stringPath;

		QObject::connect(
		    active,
		    &NMActiveConnectionAdapter::ready,
		    this,
		    [this, active]() {
			    QString path = active->connection().path();
			    if (!this->mConnectionMap.contains(path)) {
				    this->registerConnection(path);
			    }
			    emit this->activeConnectionLoaded(active);
		    },
		    Qt::SingleShotConnection
		);
	}
}

void NMDeviceAdapter::onAvailableConnectionsChanged(const QList<QDBusObjectPath>& paths) {
	QSet<QString> newConnectionPaths;
	for (const QDBusObjectPath& path: paths) {
		newConnectionPaths.insert(path.path());
	}

	QSet<QString> addedConnections = newConnectionPaths - this->mConnectionPaths;
	QSet<QString> removedConnections = this->mConnectionPaths - newConnectionPaths;
	for (const QString& path: addedConnections) {
		registerConnection(path);
	}
	for (const QString& path: removedConnections) {
		auto* connection = this->mConnectionMap.take(path);
		if (!connection) {
			qCDebug(logNetworkManager) << "NetworkManager backend sent removal signal for" << path
			                           << "which is not registered.";
		} else {
			emit this->connectionRemoved(connection);
			delete connection;
		}
		this->mConnectionPaths.remove(path);
	};
}

void NMDeviceAdapter::registerConnection(const QString& path) {
	auto* connection = new NMConnectionSettingsAdapter(path, this);
	if (!connection->isValid()) {
		qCWarning(logNetworkManager) << "Ignoring invalid registration of" << path;
		delete connection;
	} else {
		this->mConnectionMap.insert(path, connection);
		this->mConnectionPaths.insert(path);
		QObject::connect(
		    connection,
		    &NMConnectionSettingsAdapter::ready,
		    this,
		    [this, connection]() { emit this->connectionLoaded(connection); },
		    Qt::SingleShotConnection
		);
		qCDebug(logNetworkManager) << "Registered connection" << path;
	}
}

void NMDeviceAdapter::disconnect() { this->proxy->Disconnect(); }
bool NMDeviceAdapter::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMDeviceAdapter::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMDeviceAdapter::path() const { return this->proxy ? this->proxy->path() : QString(); }

} // namespace qs::network
