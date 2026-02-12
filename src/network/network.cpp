#include "network.hpp"
#include <utility>

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
#include "nm_settings.hpp"

namespace qs::network {

namespace {
QS_LOGGING_CATEGORY(logNetwork, "quickshell.network", QtWarningMsg);
} // namespace

Networking::Networking(QObject* parent): QObject(parent) {
	// Try to create the NetworkManager backend and bind to it.
	auto* nm = new NetworkManager(this);
	if (nm->isAvailable()) {
		QObject::connect(nm, &NetworkManager::deviceAdded, this, &Networking::deviceAdded);
		QObject::connect(nm, &NetworkManager::deviceRemoved, this, &Networking::deviceRemoved);
		QObject::connect(this, &Networking::requestSetWifiEnabled, nm, &NetworkManager::setWifiEnabled);
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

void Networking::deviceAdded(NetworkDevice* dev) { this->mDevices.insertObject(dev); }
void Networking::deviceRemoved(NetworkDevice* dev) { this->mDevices.removeObject(dev); }

void Networking::setWifiEnabled(bool enabled) {
	if (this->bWifiEnabled == enabled) return;
	emit this->requestSetWifiEnabled(enabled);
}

Network::Network(QString name, QObject* parent): QObject(parent), mName(std::move(name)) {
	this->bStateChanging.setBinding([this] {
		auto state = this->bState.value();
		return state == NetworkState::Connecting || state == NetworkState::Disconnecting;
	});
};

void Network::connect() {
	if (this->bConnected) {
		qCCritical(logNetwork) << this << "is already connected.";
		return;
	}

	this->requestConnect();
}

void Network::connectWithSettings(NMSettings* settings) {
	if (this->bConnected) {
		qCCritical(logNetwork) << this << "is already connected.";
		return;
	}

	if (this->mNmSettings.indexOf(settings) == -1) return;
	this->requestConnectWithSettings(settings);
}

void Network::disconnect() {
	if (!this->bConnected) {
		qCCritical(logNetwork) << this << "is not currently connected";
		return;
	}

	this->requestDisconnect();
}

void Network::forget() { this->requestForget(); }

void Network::settingsAdded(NMSettings* settings) { this->mNmSettings.insertObject(settings); }
void Network::settingsRemoved(NMSettings* settings) { this->mNmSettings.removeObject(settings); }

NMConnectionContext::NMConnectionContext(QObject* parent): QObject(parent) {}

void NMConnectionContext::setNetwork(Network* network) {
	if (this->bNetwork == network) return;
	if (this->bNetwork) disconnect(this->bNetwork, nullptr, this, nullptr);
	this->bNetwork = network;
	if (!network) return;

	QObject::connect(network, &Network::activeSettingsChanged, this, [network, this]() {
		if (network->activeSettings()) {
			if (this->bSettings) disconnect(this->bSettings, nullptr, this, nullptr);
			this->bSettings = network->activeSettings();
			QObject::connect(this->bSettings, &NMSettings::destroyed, this, [this]() {
				this->bSettings = nullptr;
			});
		};
	});
	QObject::connect(network, &Network::stateChanged, this, [network, this]() {
		if (network->state() == NetworkState::Connected) emit this->activated();
		if (network->state() == NetworkState::Connecting) emit this->activating();
	});
	QObject::connect(network, &Network::nmStateReasonChanged, this, [network, this]() {
		// "Device disconnected" isn't useful, so we defer to the last device fail reason.
		if (network->nmStateReason() == NMNetworkStateReason::DeviceDisconnected) {
			auto failReason = network->nmDeviceFailReason();
			if (failReason == NMDeviceStateReason::NoSecrets) emit this->noSecrets();
			if (failReason == NMDeviceStateReason::SupplicantDisconnect)
				emit this->supplicantDisconnect();
			if (failReason == NMDeviceStateReason::SupplicantFailed) emit this->supplicantFailed();
			if (failReason == NMDeviceStateReason::SupplicantTimeout) emit this->supplicantTimeout();
		};
	});

	connect(network, &Network::destroyed, this, [this]() { this->bNetwork = nullptr; });
}

} // namespace qs::network
