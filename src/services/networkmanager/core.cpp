#include <qdbusconnection.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qdbusextratypes.h>

#include "core.hpp"
#include "dbus_service.h"

namespace qs::service::networkmanager {

namespace {
Q_LOGGING_CATEGORY(logNetworkManager, "quickshell.service.networkmanager", QtDebugMsg);
}

NetworkManager::NetworkManager(){
	auto bus = QDbusConnection::systemBus();

	if (!bus.isConnected()) {
		// Log message
		qCWarning(logNetworkManager) << "Could not connect to DBus. NetworkManager service will not work.";
		return;
	}

	this->service = new DBusNetworkManagerService("org.freedesktop.NetworkManager", "/org/freedesktop/NetworkManager", bus, this);
	
	if (!this->service->isValid()) {
		qCDebug(logNetworkManager) << "NetworkManager service is not currently running, attempting to start it.";

		dbus::tryLaunchService(this, bus, "org.freedesktop.NetworkManager", [this](bool success) {
			if (success) {
				qCDebug(logNetworkManager) << "Successfully launched NetworkManager service.";
				this->init();
			} else {
				qCWarning(logNetworkManager) << "Could not start NetworkManager. The NetworkManager service will not work.";
			}
		});
	} else {
		this->init();
	}
}

NetworkManager* NetworkManager::instance() {
	static NetworkManager* instance = new NetworkManager(); // NOLINT
	return instance;
}

void NetworkManager::init() {
	qCDebug(logNetworkManager) << "Hi mom";
}

}
