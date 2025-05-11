#include "core.hpp"
#include <qdbusconnection.h>

namespace qs::service::networkmanager {

namespace {
Q_LOGGING_CATEGORY(logNetworkManager, "quickshell.service.networkmanager", QtWarningMsg);
}

NetworkManager::NetworkManager(){
	auto bus = QDbusConnection::systemBus();

	if (!bus.isConnected()) {
		// Log message
	}
}

}
