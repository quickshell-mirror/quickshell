#include "core.hpp"

#include <qlogging.h>

#include "../../dbus/bus.hpp"
#include "dbus_service.h"

namespace qs::service::networkmanager {

const QString NM_SERVICE = "org.freedesktop.NetworkManager";
const QString NM_PATH = "/org/freedesktop/NetworkManager";

namespace {
Q_LOGGING_CATEGORY(logNetworkManager, "quickshell.service.networkmanager", QtWarningMsg);
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

void NetworkManager::init() {}

NetworkManager* NetworkManager::instance() {
	static NetworkManager* instance = nullptr;
	if (!instance) {
		instance = new NetworkManager();
	}
	return instance;
}

} // namespace qs::service::networkmanager
