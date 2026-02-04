#include "network.hpp"
#include <utility>

#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtmetamacros.h>

#include "../core/logcat.hpp"
#include "connection.hpp"
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

QString NMNetworkStateReason::toString(NMNetworkStateReason::Enum reason) {
	switch (reason) {
	case Unknown: return QStringLiteral("Unknown");
	case None: return QStringLiteral("No reason");
	case UserDisconnected: return QStringLiteral("User disconnection");
	case DeviceDisconnected:
		return QStringLiteral("The device the connection was using was disconnected.");
	case ServiceStopped:
		return QStringLiteral("The service providing the VPN connection was stopped.");
	case IpConfigInvalid:
		return QStringLiteral("The IP config of the active connection was invalid.");
	case ConnectTimeout:
		return QStringLiteral("The connection attempt to the VPN service timed out.");
	case ServiceStartTimeout:
		return QStringLiteral(
		    "A timeout occurred while starting the service providing the VPN connection."
		);
	case ServiceStartFailed:
		return QStringLiteral("Starting the service providing the VPN connection failed.");
	case NoSecrets: return QStringLiteral("Necessary secrets for the connection were not provided.");
	case LoginFailed: return QStringLiteral("Authentication to the server failed.");
	case ConnectionRemoved:
		return QStringLiteral("Necessary secrets for the connection were not provided.");
	case DependencyFailed:
		return QStringLiteral("Master connection of this connection failed to activate.");
	case DeviceRealizeFailed: return QStringLiteral("Could not create the software device link.");
	case DeviceRemoved: return QStringLiteral("The device this connection depended on disappeared.");
	default: return QStringLiteral("Unknown");
	};
};

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

void Network::setNmDefaultConnection(NMConnection* conn) {
	if (this->bNmDefaultConnection == conn) return;
	if (!this->mNmConnections.valueList().contains(conn)) return;
	emit this->requestSetNmDefaultConnection(conn);
}

void Network::connect() {
	if (this->bConnected) {
		qCCritical(logNetwork) << this << "is already connected.";
		return;
	}

	this->requestConnect();
}

void Network::disconnect() {
	if (!this->bConnected) {
		qCCritical(logNetwork) << this << "is not currently connected";
		return;
	}

	this->requestDisconnect();
}

void Network::forget() { this->requestForget(); }

void Network::connectionAdded(NMConnection* conn) { this->mNmConnections.insertObject(conn); }
void Network::connectionRemoved(NMConnection* conn) { this->mNmConnections.removeObject(conn); }

NMConnectionContext::NMConnectionContext(QObject* parent): QObject(parent) {}

void NMConnectionContext::setNetwork(Network* network) {
	if (this->bNetwork == network) return;
	if (this->bNetwork) disconnect(this->bNetwork, nullptr, this, nullptr);
	this->bNetwork = network;

	connect(network, &Network::stateChanged, this, [network, this]() {
		if (network->state() == NetworkState::Connected) emit this->success();
	});
	connect(network, &Network::stateReasonChanged, this, [network, this]() {
		if (network->stateReason() == NMNetworkStateReason::NoSecrets) emit this->noSecrets();
		if (network->stateReason() == NMNetworkStateReason::LoginFailed) emit this->loginFailed();
	});
	connect(network, &Network::destroyed, this, [this]() { this->bNetwork = nullptr; });
}

} // namespace qs::network
