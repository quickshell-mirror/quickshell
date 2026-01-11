#include "network.hpp"
#include <utility>

#include <qhashfunctions.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include "../core/logcat.hpp"
#include "device.hpp"
#include "nm/backend.hpp"

namespace qs::network {

namespace {
QS_LOGGING_CATEGORY(logNetwork, "quickshell.network", QtWarningMsg);
} // namespace

QString NetworkState::toString(NetworkState::Enum state) {
	switch (state) {
	case NetworkState::Connecting: return QStringLiteral("Connecting");
	case NetworkState::Connected: return QStringLiteral("Connected");
	case NetworkState::Disconnecting: return QStringLiteral("Disconnecting");
	case NetworkState::Disconnected: return QStringLiteral("Disconnected");
	default: return QStringLiteral("Unknown");
	}
}

Network::Network(QObject* parent): QObject(parent) {
	// Try to create the NetworkManager backend and bind to it.
	auto* nm = new NetworkManager(this);
	if (nm->isAvailable()) {
		QObject::connect(nm, &NetworkManager::deviceAdded, this, &Network::deviceAdded);
		QObject::connect(nm, &NetworkManager::deviceRemoved, this, &Network::deviceRemoved);
		QObject::connect(this, &Network::requestSetWifiEnabled, nm, &NetworkManager::setWifiEnabled);
		this->bindableWifiEnabled().setBinding([nm]() { return nm->wifiEnabled(); });
		this->bindableWifiHardwareEnabled().setBinding([nm]() { return nm->wifiHardwareEnabled(); });

		this->mBackend = nm;
		this->mBackendType = NetworkBackendType::NetworkManager;
		return;
	} else {
		delete nm;
	}

	qCCritical(logNetwork) << "Network will not work. Could not find an available backend.";
}

void Network::deviceAdded(NetworkDevice* dev) { this->mDevices.insertObject(dev); }
void Network::deviceRemoved(NetworkDevice* dev) { this->mDevices.removeObject(dev); }

void Network::setWifiEnabled(bool enabled) {
	if (this->bWifiEnabled == enabled) return;
	emit this->requestSetWifiEnabled(enabled);
}

BaseNetwork::BaseNetwork(QString name, QObject* parent): QObject(parent), mName(std::move(name)) {
	this->bStateChanging.setBinding([this] {
		auto state = this->bState.value();
		return state == NetworkState::Connecting || state == NetworkState::Disconnecting;
	});
};

} // namespace qs::network
