#include "api.hpp"

#include <qdbusconnection.h>
#include <qdbusconnectioninterface.h>
#include <qdbusextratypes.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qdbusservicewatcher.h>
#include <qlogging.h>

#include "nm_backend.hpp"

namespace qs::network {

namespace {
Q_LOGGING_CATEGORY(logNetworkNetworkDevice, "quickshell.network.device", QtWarningMsg);
Q_LOGGING_CATEGORY(logNetwork, "quickshell.network", QtWarningMsg);
} // namespace

NetworkDevice::NetworkDevice(QObject* parent): QObject(parent) {};
WirelessNetworkDevice::WirelessNetworkDevice(QObject* parent): NetworkDevice(parent) {};

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
	case NetworkDeviceType::Ethernet: return QStringLiteral("Ethernet");
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
		qCCritical(logNetworkNetworkDevice) << "NetworkDevice" << this << "is already disconnected";
		return;
	}

	if (this->bState == NetworkDeviceState::Disconnecting) {
		qCCritical(logNetworkNetworkDevice) << "NetworkDevice" << this << "is already disconnecting";
		return;
	}

	qCDebug(logNetworkNetworkDevice) << "Disconnecting from device" << this;

	signalDisconnect();
}

void WirelessNetworkDevice::scanComplete(qint64 lastScan) {
	this->bLastScan = lastScan;
	emit this->lastScanChanged();

	if (this->bScanning) {
		this->bScanning = false;
		emit this->scanningChanged();
	}
}

void WirelessNetworkDevice::scan() {
	if (this->bScanning) {
		qCCritical(logNetworkNetworkDevice) << "NetworkDevice" << this << "is already scanning";
		return;
	}

	qCDebug(logNetworkNetworkDevice) << "Requesting scan on wireless device" << this;
	this->bScanning = true;
	signalScan();
}

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
