#include "device.hpp"

#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstringliteral.h>

#include "../core/logcat.hpp"

namespace qs::network {

namespace {
QS_LOGGING_CATEGORY(logNetworkDevice, "quickshell.network.device", QtWarningMsg);
} // namespace

QString DeviceConnectionState::toString(DeviceConnectionState::Enum state) {
	switch (state) {
	case Unknown: return QStringLiteral("Unknown");
	case Connecting: return QStringLiteral("Connecting");
	case Connected: return QStringLiteral("Connected");
	case Disconnecting: return QStringLiteral("Disconnecting");
	case Disconnected: return QStringLiteral("Disconnected");
	}
}

QString DeviceType::toString(DeviceType::Enum type) {
	switch (type) {
	case None: return QStringLiteral("None");
	case Wifi: return QStringLiteral("Wifi");
	}
}

NetworkDevice::NetworkDevice(DeviceType::Enum type, QObject* parent)
    : QObject(parent)
    , mType(type) {};

void NetworkDevice::disconnect() {
	if (this->bState == DeviceConnectionState::Disconnected) {
		qCCritical(logNetworkDevice) << "Device" << this << "is already disconnected";
		return;
	}
	if (this->bState == DeviceConnectionState::Disconnecting) {
		qCCritical(logNetworkDevice) << "Device" << this << "is already disconnecting";
		return;
	}
	qCDebug(logNetworkDevice) << "Disconnecting from device" << this;
	this->requestDisconnect();
}

} // namespace qs::network
