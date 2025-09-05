#include "network.hpp"

#include <qlogging.h>
#include <qloggingcategory.h>

#include "../core/logcat.hpp"
#include "nm/backend.hpp"

namespace qs::network {

namespace {
QS_LOGGING_CATEGORY(logNetwork, "quickshell.network", QtWarningMsg);
} // namespace

Network::Network(QObject* parent): QObject(parent), mWifi(new Wifi(this)) {
	// NetworkManager
	auto* nm = new NetworkManager(this);
	if (nm->isAvailable()) {
		// clang-format off
		QObject::connect(nm, &NetworkManager::wifiDeviceAdded, this->wifi(), &Wifi::onDeviceAdded);
		QObject::connect(nm, &NetworkManager::wifiDeviceRemoved, this->wifi(), &Wifi::onDeviceRemoved);
		this->wifi()->bindableEnabled().setBinding([nm]() { return nm->wifiEnabled(); });
		this->wifi()->bindableHardwareEnabled().setBinding([nm]() { return nm->wifiHardwareEnabled(); });
		QObject::connect(this->wifi(), &Wifi::requestSetEnabled, nm, &NetworkManager::setWifiEnabled);
		// clang-format on
		this->mBackend = nm;
		this->mBackendType = NetworkBackendType::NetworkManager;
		return;
	} else {
		delete nm;
	}

	qCCritical(logNetwork) << "Network will not work. Could not find an available backend.";
}

} // namespace qs::network
