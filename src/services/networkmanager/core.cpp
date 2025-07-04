#include "core.hpp"

#include <qdbusconnection.h>
#include <qdbusconnectioninterface.h>
#include <qdbusextratypes.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qdbusservicewatcher.h>
#include <qlogging.h>

#include "../../dbus/bus.hpp"
#include "../../dbus/properties.hpp"
#include "dbus_service.h"

namespace qs::service::networkmanager {

const QString NM_SERVICE = "org.freedesktop.NetworkManager";
const QString NM_PATH = "/org/freedesktop/NetworkManager";

namespace {
Q_LOGGING_CATEGORY(logNetworkManager, "quickshell.service.networkmanager", QtWarningMsg);
}

QString NetworkManagerState::toString(NetworkManagerState::Enum state) {
	auto metaEnum = QMetaEnum::fromType<NetworkManagerState::Enum>();
	if (metaEnum.valueToKey(state)) {
		return QString(metaEnum.valueToKey(state));
	}
	return "Invalid state";
}

NetworkManager::NetworkManager() {
	qCDebug(logNetworkManager) << "Starting NetworkManager Service";

	auto bus = QDBusConnection::systemBus();
	if (!bus.isConnected()) {
		qCWarning(logNetworkManager
		) << "Could not connect to DBus. NetworkManager service will not work.";
		return;
	}

	this->service = new DBusNetworkManagerService(NM_SERVICE, NM_PATH, bus, this);

	if (!this->service->isValid()) {
		qCDebug(logNetworkManager
		) << "NetworkManager service is not currently running, attempting to start it.";

		dbus::tryLaunchService(this, bus, NM_SERVICE, [this](bool success) {
			if (success) {
				qCDebug(logNetworkManager) << "Successfully launched NetworkManager service.";
				this->init();
			} else {
				qCWarning(logNetworkManager)
				    << "Could not start NetworkManager. The NetworkManager service will not work.";
			}
		});
	} else {
		this->init();
	}
}

void NetworkManager::init() {
	// QObject::connect(
	//     this->service,
	//     &DBusNetworkManagerService::DeviceAdded,
	//     this,
	//     &NetworkManager::onDeviceAdded
	// );
	//
	// QObject::connect(
	//     this->service,
	//     &DBusNetworkManagerService::DeviceRemoved,
	//     this,
	//     &NetworkManager::onDeviceRemoved
	// );

	this->serviceProperties.setInterface(this->service);
	this->serviceProperties.updateAllViaGetAll();

	// this->registerDevices();
}

// void NetworkManager::registerDevices() {
// 	auto pending = this->service->GetDevices();
// 	auto* call = new QDBusPendingCallWatcher(pending, this);
//
// 	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
// 		const QDBusPendingReply<QList<QDBusObjectPath>> reply = *call;
//
// 		if (reply.isError()) {
// 			qCWarning(logNetworkManager) << "Failed to get devices: " << reply.error().message();
// 		} else {
// 			for (const QDBusObjectPath& devicePath: reply.value()) {
// 				qCDebug(logNetworkManager) << "Device added:" << devicePath.path();
// 			}
// 		}
//
// 		delete call;
// 	};
//
// 	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
// }
//
// void NetworkManager::onDeviceAdded(const QDBusObjectPath& path) {
// 	qCDebug(logNetworkManager) << "Device added:" << path;
// }
//
// void NetworkManager::onDeviceRemoved(const QDBusObjectPath& path) {
// 	qCDebug(logNetworkManager) << "Device removed:" << path;
// }

NetworkManager* NetworkManager::instance() {
	static NetworkManager* instance = nullptr;
	if (!instance) {
		instance = new NetworkManager();
	}
	return instance;
}

NetworkManagerQml::NetworkManagerQml(QObject* parent): QObject(parent) {
	QObject::connect(
		NetworkManager::instance(),
		&NetworkManager::stateChanged,
		this,
		&NetworkManagerQml::stateChanged
	);
}

} // namespace qs::service::networkmanager

namespace qs::dbus {
using namespace qs::service::networkmanager;

DBusResult<NetworkManagerState::Enum>
DBusDataTransform<NetworkManagerState::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<NetworkManagerState::Enum>(wire));
}

} // namespace qs::dbus
