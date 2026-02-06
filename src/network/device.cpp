#include "device.hpp"

#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtmetamacros.h>

#include "../core/logcat.hpp"
#include "enums.hpp"

namespace qs::network {

namespace {
QS_LOGGING_CATEGORY(logNetworkDevice, "quickshell.network.device", QtWarningMsg);
} // namespace

NetworkDevice::NetworkDevice(DeviceType::Enum type, QObject* parent): QObject(parent), mType(type) {
	this->bindableConnected().setBinding([this]() {
		return this->bState == DeviceConnectionState::Connected;
	});
};

void NetworkDevice::setAutoconnect(bool autoconnect) {
	if (this->bAutoconnect == autoconnect) return;
	emit this->requestSetAutoconnect(autoconnect);
}

void NetworkDevice::setNmManaged(bool managed) {
	if (this->bNmManaged == managed) return;
	emit this->requestSetNmManaged(managed);
}

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
