#include "device.hpp"

#include <qdbusconnection.h>
#include <qdbusextratypes.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/logcat.hpp"
#include "../../dbus/properties.hpp"
#include "../device.hpp"
#include "../enums.hpp"
#include "active_connection.hpp"
#include "connection.hpp"
#include "dbus_nm_device.h"
#include "dbus_types.hpp"
#include "enums.hpp"
#include "network.hpp"

namespace qs::network {
using namespace qs::dbus;

namespace {
QS_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
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
	QObject::connect(this, &NMDevice::activeConnectionPathChanged, this, &NMDevice::onActiveConnectionPathChanged);
	QObject::connect(this, &NMDevice::activeConnectionLoaded, this, &NMDevice::onActiveConnectionLoaded);
	QObject::connect(this->deviceProxy, &DBusNMDeviceProxy::StateChanged, this, &NMDevice::onStateChanged);
	// clang-format on

	this->deviceProperties.setInterface(this->deviceProxy);
	this->deviceProperties.updateAllViaGetAll();
}

void NMDevice::bindFrontend(NetworkDevice* frontend) {
	auto translateState = [this]() {
		switch (this->state()) {
		case 0 ... 20: return ConnectionState::Unknown;
		case 30: return ConnectionState::Disconnected;
		case 40 ... 90: return ConnectionState::Connecting;
		case 100: return ConnectionState::Connected;
		case 110 ... 120: return ConnectionState::Disconnecting;
		}
	};
	// clang-format off
	frontend->bindableName().setBinding([this]() { return this->interface(); });
	frontend->bindableAddress().setBinding([this]() { return this->hwAddress(); });
	frontend->bindableState().setBinding(translateState);
	frontend->bindableAutoconnect().setBinding([this]() { return this->autoconnect(); });
	frontend->bindableNmManaged().setBinding([this]() { return this->managed(); });
	QObject::connect(frontend, &NetworkDevice::requestDisconnect, this, &NMDevice::disconnect);
	QObject::connect(frontend, &NetworkDevice::requestSetAutoconnect, this, &NMDevice::setAutoconnect);
	QObject::connect(frontend, &NetworkDevice::requestSetNmManaged, this, &NMDevice::setManaged);
	QObject::connect(this, &NMDevice::networkAdded, frontend, &NetworkDevice::networkAdded);
	QObject::connect(this, &NMDevice::networkRemoved, frontend, &NetworkDevice::networkRemoved);
}

void NMDevice::onStateChanged(quint32 newState, quint32 /*oldState*/, quint32 reason) {
	auto enumReason = static_cast<NMDeviceStateReason::Enum>(reason);
	auto enumNewState = static_cast<NMDeviceState::Enum>(newState);
	if (enumNewState == NMDeviceState::Failed) this->bLastFailReason = enumReason;
	if (this->bStateReason == enumReason) return;
	this->bStateReason = enumReason;
}

void NMDevice::onConnectionLoaded(NMConnection* conn) {
	QObject::connect(conn, &NMConnection::settingsChanged, this, [this, conn]() {
		this->assignToNetwork(conn);
	});
	QObject::connect(this, &NMDevice::availableConnectionsChanged, conn, [this, conn]() {
		this->assignToNetwork(conn);
	});
	QObject::connect(conn, &NMConnection::unregistered, this, [this, conn]() {
		if (auto* net = this->mConnectionNetworks.take(conn)) net->removeConnection(conn);
	});
	this->assignToNetwork(conn);

	// The NMActiveConnection may have loaded before its NMConnection
	auto* active = this->mActiveConnection;
	if (active && active->connection().path() == conn->path()) {
		if (auto* net = this->mConnectionNetworks.value(conn)) net->addActiveConnection(active);
	}
}

void NMDevice::assignToNetwork(NMConnection* conn) {
	auto* current = this->mConnectionNetworks.value(conn);
	auto* target = this->bAvailableConnections.value().contains(QDBusObjectPath(conn->path()))
	                 ? this->networkForConnection(conn)
	                 : nullptr;
	if (current == target) return;

	if (target) this->mConnectionNetworks.insert(conn, target);
	else this->mConnectionNetworks.remove(conn);
	if (current) current->removeConnection(conn);

	if (target) target->addConnection(conn);
}

void NMDevice::bindNetwork(NMNetwork* net) {
	net->bindableDeviceFailReason().setBinding([this]() { return this->lastFailReason(); });
	QObject::connect(net, &NMNetwork::requestDisconnect, this, &NMDevice::disconnect);
	QObject::connect(net, &NMNetwork::requestActivateConnection, this, [this](const QString& settingsPath){
		emit this->activateConnection(QDBusObjectPath(settingsPath), QDBusObjectPath(this->path()));	
	});
	QObject::connect(net, &NMNetwork::requestAddAndActivateConnection, this, [this](const NMSettings& settingsMap, const QString& specificObject){
		emit this->addAndActivateConnection(settingsMap, QDBusObjectPath(this->path()), QDBusObjectPath(specificObject));	
	});
	QObject::connect(net, &NMNetwork::visibilityChanged, this, [this, net](bool visible) {
		if (visible) emit this->networkAdded(net->frontend());
		else emit this->networkRemoved(net->frontend());
	});
	if (net->visible()) emit this->networkAdded(net->frontend());
}

void NMDevice::onActiveConnectionPathChanged(const QDBusObjectPath& path) {
	const QString stringPath = path.path();

	// Remove old active connection
	if (this->mActiveConnection) {
		qCDebug(logNetworkManager) << "Active connection removed:" << this->mActiveConnection->path();
		QObject::disconnect(this->mActiveConnection, nullptr, this, nullptr);
		delete this->mActiveConnection;
		this->mActiveConnection = nullptr;
	}

	// Create new active connection
	if (stringPath != "/") {
		auto* active = new NMActiveConnection(stringPath, this);
		if (!active->isValid()) {
			qCWarning(logNetworkManager) << "Ignoring invalid registration of" << stringPath;
			delete active;
		} else {
			qCDebug(logNetworkManager) << "Active connection added:" << stringPath;
			this->mActiveConnection = active;
			QObject::connect(
			    active,
			    &NMActiveConnection::loaded,
			    this,
			    [this, active]() { emit this->activeConnectionLoaded(active); },
			    Qt::SingleShotConnection
			);
		}
	}
}

void NMDevice::onActiveConnectionLoaded(NMActiveConnection* active) {
	const auto connectionPath = active->connection().path();
	for (auto it = this->mConnectionNetworks.cbegin(); it != this->mConnectionNetworks.cend(); ++it) {
		if (it.key()->path() == connectionPath) {
			it.value()->addActiveConnection(active);
			return;
		}
	}
}

void NMDevice::disconnect() { this->deviceProxy->Disconnect(); }

void NMDevice::setAutoconnect(bool autoconnect) {
	if (autoconnect == this->bAutoconnect) return;
	this->bAutoconnect = autoconnect;
	this->pAutoconnect.write();
}

void NMDevice::setManaged(bool managed) {
	if (managed == this->bManaged) return;
	this->bManaged = managed;
	this->pManaged.write();
}

bool NMDevice::isValid() const { return this->deviceProxy && this->deviceProxy->isValid(); }
QString NMDevice::address() const {
	return this->deviceProxy ? this->deviceProxy->service() : QString();
}
QString NMDevice::path() const { return this->deviceProxy ? this->deviceProxy->path() : QString(); }

} // namespace qs::network

namespace qs::dbus {

DBusResult<qs::network::NMDeviceState::Enum>
DBusDataTransform<qs::network::NMDeviceState::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<qs::network::NMDeviceState::Enum>(wire));
}

DBusResult<qs::network::NMDeviceInterfaceFlags::Enum>
DBusDataTransform<qs::network::NMDeviceInterfaceFlags::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<qs::network::NMDeviceInterfaceFlags::Enum>(wire));
}

} // namespace qs::dbus
