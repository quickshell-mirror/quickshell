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
	QObject::connect(&this->deviceProperties, &DBusPropertyGroup::getAllFinished, this, [this]() { emit this->ready(); }, Qt::SingleShotConnection);
	// clang-format on

	this->deviceProperties.setInterface(this->proxy);
	this->deviceProperties.updateAllViaGetAll();
}

void NMDeviceAdapter::onAvailableConnectionsChanged(const QList<QDBusObjectPath>& paths) {
	QSet<QString> newConnectionPaths;
	for (const QDBusObjectPath& path: paths) {
		newConnectionPaths.insert(path.path());
	}

	QSet<QString> addedConnections = newConnectionPaths - this->mConnectionPaths;
	QSet<QString> removedConnections = this->mConnectionPaths - newConnectionPaths;
	for (const QString& path: addedConnections) {
		auto* connection = new NMConnectionAdapter(path, this);
		if (!connection->isValid()) {
			qCWarning(logNetworkManager) << "Ignoring invalid registration of" << path;
			delete connection;
		} else {
			this->mConnectionMap.insert(path, connection);
			QObject::connect(
			    connection,
			    &NMConnectionAdapter::ready,
			    this,
			    [this, connection]() { emit this->connectionLoaded(connection); },
			    Qt::SingleShotConnection
			);
			qCDebug(logNetworkManager) << "Registered connection" << path;
		}
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
	};
}

void NMDeviceAdapter::disconnect() { this->proxy->Disconnect(); }
bool NMDeviceAdapter::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMDeviceAdapter::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMDeviceAdapter::path() const { return this->proxy ? this->proxy->path() : QString(); }

} // namespace qs::network
