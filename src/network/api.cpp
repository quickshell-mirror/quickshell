#include "api.hpp"

#include <qdbusconnection.h>
#include <qdbusconnectioninterface.h>
#include <qdbusextratypes.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qdbusservicewatcher.h>
#include <qlogging.h>

#include "nm/backend.hpp"

namespace qs::network {

namespace {
Q_LOGGING_CATEGORY(logNetwork, "quickshell.network", QtWarningMsg);
}

Device::Device(QObject* parent): QObject(parent) {};

Network::Network(QObject* parent): QObject(parent) {
	// Try each backend

	// NetworkManager
	auto* nm = new NetworkManager();
	if (nm->isAvailable()) {
		this->backend = nm;
		return;
	}

	// None found
	this->backend = nullptr;
	qCDebug(logNetwork) << "Network will not work. Could not find an available backend.";
}

} // namespace qs::network
