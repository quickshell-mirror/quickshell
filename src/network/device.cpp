#include "device.hpp"

#include <qlogging.h>
#include <qloggingcategory.h>

#include "../core/logcat.hpp"

namespace qs::network {

namespace {
QS_LOGGING_CATEGORY(logNetworkDevice, "quickshell.network.device", QtWarningMsg);
} // namespace

NetworkDevice::NetworkDevice(QObject* parent): QObject(parent) {};

void NetworkDevice::disconnect() {
	if (this->bState == NetworkConnectionState::Disconnected) {
		qCCritical(logNetworkDevice) << "Device" << this << "is already disconnected";
		return;
	}
	if (this->bState == NetworkConnectionState::Disconnecting) {
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
