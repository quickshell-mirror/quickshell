#include "qml.hpp"

#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtmetamacros.h>

#include "../core/logcat.hpp"
#include "device.hpp"
#include "enums.hpp"
#include "nm/backend.hpp"

namespace qs::network {

namespace {
QS_LOGGING_CATEGORY(logNetwork, "quickshell.network", QtWarningMsg);
} // namespace

Networking::Networking(QObject* parent): QObject(parent) {
	// Try to create the NetworkManager backend and bind to it.
	auto* nm = new NetworkManager(this);
	if (nm->isAvailable()) {
		// clang-format off
		QObject::connect(nm, &NetworkManager::deviceAdded, this, &Networking::deviceAdded);
		QObject::connect(nm, &NetworkManager::deviceRemoved, this, &Networking::deviceRemoved);
		QObject::connect(this, &Networking::requestSetWifiEnabled, nm, &NetworkManager::setWifiEnabled);
		QObject::connect(this, &Networking::requestSetConnectivityCheckEnabled, nm, &NetworkManager::setConnectivityCheckEnabled);
		QObject::connect(this, &Networking::requestCheckConnectivity, nm, &NetworkManager::checkConnectivity);
		this->bindableWifiEnabled().setBinding([nm]() { return nm->wifiEnabled(); });
		this->bindableWifiHardwareEnabled().setBinding([nm]() { return nm->wifiHardwareEnabled(); });
		this->bindableCanCheckConnectivity().setBinding([nm]() { return nm->connectivityCheckAvailable(); });
		this->bindableConnectivityCheckEnabled().setBinding([nm]() { return nm->connectivityCheckEnabled(); });
		this->bindableConnectivity().setBinding([nm]() { return static_cast<NetworkConnectivity::Enum>(nm->connectivity()); });
		// clang-format on

		this->mBackend = nm;
		this->mBackendType = NetworkBackendType::NetworkManager;
		return;
	} else {
		delete nm;
	}
	qCCritical(logNetwork) << "Network will not work. Could not find an available backend.";
}

Networking* Networking::instance() {
	static Networking* instance = new Networking(); // NOLINT
	return instance;
}

void Networking::deviceAdded(NetworkDevice* dev) { this->mDevices.insertObject(dev); }
void Networking::deviceRemoved(NetworkDevice* dev) { this->mDevices.removeObject(dev); }

void Networking::checkConnectivity() {
	if (!this->bConnectivityCheckEnabled || !this->bCanCheckConnectivity) return;
	emit this->requestCheckConnectivity();
}

void Networking::setWifiEnabled(bool enabled) {
	if (this->bWifiEnabled == enabled) return;
	emit this->requestSetWifiEnabled(enabled);
}

void Networking::setConnectivityCheckEnabled(bool enabled) {
	if (this->bConnectivityCheckEnabled == enabled) return;
	emit this->requestSetConnectivityCheckEnabled(enabled);
}

NetworkingQml::NetworkingQml(QObject* parent): QObject(parent) {
	// clang-format off
	QObject::connect(Networking::instance(), &Networking::wifiEnabledChanged, this, &NetworkingQml::wifiEnabledChanged);
	QObject::connect(Networking::instance(), &Networking::wifiHardwareEnabledChanged, this, &NetworkingQml::wifiHardwareEnabledChanged);
	QObject::connect(Networking::instance(), &Networking::canCheckConnectivityChanged, this, &NetworkingQml::canCheckConnectivityChanged);
	QObject::connect(Networking::instance(), &Networking::connectivityCheckEnabledChanged, this, &NetworkingQml::connectivityCheckEnabledChanged);
	QObject::connect(Networking::instance(), &Networking::connectivityChanged, this, &NetworkingQml::connectivityChanged);
	// clang-format on
}

void NetworkingQml::checkConnectivity() { Networking::instance()->checkConnectivity(); }

} // namespace qs::network
