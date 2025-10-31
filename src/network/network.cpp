#include "network.hpp"
#include <utility>

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

Network::Network(QObject* parent): QObject(parent) {
	// NetworkManager
	auto* nm = new NetworkManager(this);
	if (nm->isAvailable()) {
		QObject::connect(nm, &NetworkManager::deviceAdded, this, &Network::onDeviceAdded);
		QObject::connect(nm, &NetworkManager::deviceRemoved, this, &Network::onDeviceRemoved);
		this->bindableWifiEnabled().setBinding([nm]() { return nm->wifiEnabled(); });
		this->bindableWifiHardwareEnabled().setBinding([nm]() { return nm->wifiHardwareEnabled(); });
		QObject::connect(this, &Network::requestSetWifiEnabled, nm, &NetworkManager::setWifiEnabled);
		this->mBackend = nm;
		this->mBackendType = NetworkBackendType::NetworkManager;
		return;
	} else {
		delete nm;
	}

	qCCritical(logNetwork) << "Network will not work. Could not find an available backend.";
}

void Network::onDeviceAdded(NetworkDevice* dev) { this->mDevices.insertObject(dev); }
void Network::onDeviceRemoved(NetworkDevice* dev) { this->mDevices.removeObject(dev); }

void Network::setWifiEnabled(bool enabled) {
	if (this->bWifiEnabled == enabled) {
		const QString state = enabled ? "enabled" : "disabled";
		qCCritical(logNetwork) << "Wifi is already software" << state;
	} else {
		emit this->requestSetWifiEnabled(enabled);
	}
}

BaseNetwork::BaseNetwork(QString name, QObject* parent): QObject(parent), mName(std::move(name)) {};

} // namespace qs::network
