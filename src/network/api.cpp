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
Q_LOGGING_CATEGORY(logNetworkDevice, "quickshell.network.device", QtWarningMsg);
Q_LOGGING_CATEGORY(logNetwork, "quickshell.network", QtWarningMsg);
}

Device::Device(QObject* parent): QObject(parent) {};
WirelessDevice::WirelessDevice(QObject* parent): Device(parent) {};

QString DeviceState::toString(DeviceState::Enum state) {
	switch (state) {
	case DeviceState::Unknown: return QStringLiteral("Unknown");
	case DeviceState::Disconnected: return QStringLiteral("Disconnected");
	case DeviceState::Connecting: return QStringLiteral("Connecting");
	case DeviceState::Connected: return QStringLiteral("Connected");
	case DeviceState::Disconnecting: return QStringLiteral("Disconnecting");
	default: return QStringLiteral("Unknown");
	}
}

void Device::setName(const QString& name) {
	if (name != this->bName) {
		this->bName = name;
	}
}

void Device::setAddress(const QString& address) {
	if (address != this->bAddress) {
		this->bAddress = address;
	}
}

void Device::setState(DeviceState::Enum state) {
	if (state != this->bState) {
		this->bState = state;
	}
}

void Device::disconnect() {
	if (this->bState == DeviceState::Disconnected) {
		qCCritical(logNetworkDevice) << "Device" << this << "is already disconnected";
		return;
	}

	if (this->bState == DeviceState::Disconnecting) {
		qCCritical(logNetworkDevice) << "Device" << this << "is already disconnecting";
		return;
	}

	qCDebug(logNetworkDevice) << "Disconnecting from device" << this;
	
	signalDisconnect();
}

void WirelessDevice::setLastScan(qint64 lastScan) {
	if (lastScan != this->bLastScan) {
		this->bLastScan = lastScan;
	}
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
