#include "device.hpp"

#include <qlogging.h>
#include <qloggingcategory.h>

#include "../core/logcat.hpp"

namespace qs::network {

namespace {
QS_LOGGING_CATEGORY(logNetworkDevice, "quickshell.network.device", QtWarningMsg);
} // namespace

NetworkDevice::NetworkDevice(QObject* parent): QObject(parent) {};

void NetworkDevice::setName(const QString& name) {
	if (name == this->mName) return;
	this->mName = name;
	emit this->nameChanged();
}

void NetworkDevice::setAddress(const QString& address) {
	if (address == this->mAddress) return;
	this->mAddress = address;
	emit this->addressChanged();
}

void NetworkDevice::setState(NetworkConnectionState::Enum state) {
	if (state == this->mState) return;
	this->mState = state;
	emit this->stateChanged();
}

void NetworkDevice::setNmState(NMDeviceState::Enum nmState) {
	if (nmState == this->mNmState) return;
	this->mNmState = nmState;
	emit this->nmStateChanged();
}

void NetworkDevice::disconnect() {
	if (this->mState == NetworkConnectionState::Disconnected) {
		qCCritical(logNetworkDevice) << "Device" << this << "is already disconnected";
		return;
	}
	if (this->mState == NetworkConnectionState::Disconnecting) {
		qCCritical(logNetworkDevice) << "Device" << this << "is already disconnecting";
		return;
	}
	qCDebug(logNetworkDevice) << "Disconnecting from device" << this;
	this->requestDisconnect();
}

QString NetworkConnectionState::toString(NetworkConnectionState::Enum state) {
	switch (state) {
	case Unknown: return QStringLiteral("Unknown");
	case Connecting: return QStringLiteral("Connecting");
	case Connected: return QStringLiteral("Connected");
	case Disconnecting: return QStringLiteral("Disconnecting");
	case Disconnected: return QStringLiteral("Disconnected");
	default: return QStringLiteral("Unknown");
	}
}

} // namespace qs::network

QDebug operator<<(QDebug debug, const qs::network::NetworkDevice* device) {
	auto saver = QDebugStateSaver(debug);

	if (device) {
		debug.nospace() << "NetworkDevice(" << static_cast<const void*>(device)
		                << ", name=" << device->name() << ")";
	} else {
		debug << "BluetoothDevice(nullptr)";
	}

	return debug;
}
