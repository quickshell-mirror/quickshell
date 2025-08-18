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

NMDevice::NMDevice(const QString& path, QObject* parent): QObject(parent) {
	this->deviceProxy = new DBusNMDeviceProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->deviceProxy->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create DBus interface for device at" << path;
		return;
	}

	// clang-format off
	QObject::connect(this, &NMDevice::availableConnectionPathsChanged, this, &NMDevice::onAvailableConnectionPathsChanged);
	QObject::connect(this, &NMDevice::activeConnectionPathChanged, this, &NMDevice::onActiveConnectionPathChanged);
	QObject::connect(&this->deviceProperties, &DBusPropertyGroup::getAllFinished, this, [this]() { emit this->deviceReady(); }, Qt::SingleShotConnection);
	// clang-format on

	this->deviceProperties.setInterface(this->deviceProxy);
	this->deviceProperties.updateAllViaGetAll();
}

void NMDevice::onActiveConnectionPathChanged(const QDBusObjectPath& path) {
	QString stringPath = path.path();
	if (this->mActiveConnection) {
		QObject::disconnect(this->mActiveConnection, nullptr, this, nullptr);
		emit this->mActiveConnection->disappeared();
		delete this->mActiveConnection;
		this->mActiveConnection = nullptr;
	}

	if (stringPath != "/") {
		auto* active = new NMActiveConnection(stringPath, this);
		this->mActiveConnection = active;
		qCDebug(logNetworkManager) << "Registered active connection" << stringPath;
				QObject::connect(
		    active,
		    &NMActiveConnection::ready,
		    this,
		    [this, active]() { emit this->activeConnectionLoaded(active); },
		    Qt::SingleShotConnection
		);
	}
}

void NMDevice::onAvailableConnectionPathsChanged(const QList<QDBusObjectPath>& paths) {
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
			emit connection->disappeared();
			delete connection;
		}
		this->mConnectionPaths.remove(path);
	};
}

void NMDevice::registerConnection(const QString& path) {
	auto* connection = new NMConnectionSettings(path, this);
	if (!connection->isValid()) {
		qCWarning(logNetworkManager) << "Ignoring invalid registration of" << path;
		delete connection;
	} else {
		this->mConnectionMap.insert(path, connection);
		this->mConnectionPaths.insert(path);
		QObject::connect(
		    connection,
		    &NMConnectionSettings::ready,
		    this,
		    [this, connection]() { emit this->connectionLoaded(connection); },
		    Qt::SingleShotConnection
		);
	}
}

void NMDevice::disconnect() { this->deviceProxy->Disconnect(); }
bool NMDevice::isDeviceValid() const { return this->deviceProxy && this->deviceProxy->isValid(); }
QString NMDevice::address() const {
	return this->deviceProxy ? this->deviceProxy->service() : QString();
}
QString NMDevice::path() const { return this->deviceProxy ? this->deviceProxy->path() : QString(); }

} // namespace qs::network
