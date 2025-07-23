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
Q_LOGGING_CATEGORY(logNetworkDevice, "quickshell.network.device", QtWarningMsg);
Q_LOGGING_CATEGORY(logNetwork, "quickshell.network", QtWarningMsg);
} // namespace

NetworkDevice::NetworkDevice(QObject* parent): QObject(parent) {};

QString NetworkDeviceState::toString(NetworkDeviceState::Enum state) {
	switch (state) {
	case NetworkDeviceState::Unknown: return QStringLiteral("Unknown");
	case NetworkDeviceState::Disconnected: return QStringLiteral("Disconnected");
	case NetworkDeviceState::Connecting: return QStringLiteral("Connecting");
	case NetworkDeviceState::Connected: return QStringLiteral("Connected");
	case NetworkDeviceState::Disconnecting: return QStringLiteral("Disconnecting");
	default: return QStringLiteral("Unknown");
	}
}

QString NetworkDeviceType::toString(NetworkDeviceType::Enum type) {
	switch (type) {
	case NetworkDeviceType::Other: return QStringLiteral("Other");
	case NetworkDeviceType::Wireless: return QStringLiteral("Wireless");
	default: return QStringLiteral("Unknown");
	}
}

void NetworkDevice::setName(const QString& name) {
	if (name != this->bName) {
		this->bName = name;
	}
}

void NetworkDevice::setAddress(const QString& address) {
	if (address != this->bAddress) {
		this->bAddress = address;
	}
}

void NetworkDevice::setState(NetworkDeviceState::Enum state) {
	if (state != this->bState) {
		this->bState = state;
	}
}

void NetworkDevice::disconnect() {
	if (this->bState == NetworkDeviceState::Disconnected) {
		qCCritical(logNetworkDevice) << "Device" << this << "is already disconnected";
		return;
	}

	if (this->bState == NetworkDeviceState::Disconnecting) {
		qCCritical(logNetworkDevice) << "Device" << this << "is already disconnecting";
		return;
	}

	qCDebug(logNetworkDevice) << "Disconnecting from device" << this;

	this->requestDisconnect();
}

NetworkWifiDevice::NetworkWifiDevice(QObject* parent): NetworkDevice(parent) {};

void NetworkWifiDevice::scanComplete() {
	if (this->bScanning) {
		this->bScanning = false;
		emit this->scanningChanged();
	}
}

void NetworkWifiDevice::scan() {
	if (this->bScanning) {
		qCCritical(logNetworkDevice) << "Wireless device" << this << "is already scanning";
		return;
	}

	qCDebug(logNetworkDevice) << "Requesting scan on wireless device" << this;
	this->bScanning = true;
	this->requestScan();
}

void NetworkWifiDevice::addNetwork(WifiNetwork* network) { mNetworks.insertObject(network); }

void NetworkWifiDevice::removeNetwork(WifiNetwork* network) { mNetworks.removeObject(network); }

WifiNetwork::WifiNetwork(QObject* parent): QObject(parent) {};

void WifiNetwork::setSsid(const QString& ssid) {
	if (this->bSsid != ssid) {
		this->bSsid = ssid;
		emit this->ssidChanged();
	}
}

void WifiNetwork::setSignalStrength(quint8 signal) {
	if (this->bSignalStrength != signal) {
		this->bSignalStrength = signal;
		emit this->signalStrengthChanged();
	}
}

void WifiNetwork::setConnected(bool connected) {
	if (this->bConnected != connected) {
		this->bConnected = connected;
		emit this->connectedChanged();
	}
}

Network::Network(QObject* parent): QObject(parent) {
	// Try each backend

	// NetworkManager
	auto* nm = new NetworkManager(this);
	connect(nm, &NetworkBackend::ready, this, [this, nm](bool success) {
		if (success) {
			QObject::connect(nm, &NetworkManager::deviceAdded, this, &Network::addDevice);
			QObject::connect(nm, &NetworkManager::deviceRemoved, this, &Network::removeDevice);
			this->backend = nm;
		} else {
			delete nm;
			qCCritical(logNetwork) << "Network will not work. Could not find an available backend.";
		}
	});
}

void Network::addDevice(NetworkDevice* device) { this->mDevices.insertObject(device); }

void Network::removeDevice(NetworkDevice* device) { this->mDevices.removeObject(device); }

} // namespace qs::network
