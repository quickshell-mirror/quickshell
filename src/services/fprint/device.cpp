#include "device.hpp"

#include <qobject.h>

namespace qs::services::fprint {

namespace {
QS_LOGGING_CATEGORY(logFprintDevice, "quickshell.service.fprint.device", QtWarningMsg);
}

FprintDevice::FprintDevice(QObject* parent): QObject(parent) {}

void FprintDevice::init(const QString& path) {
	this->device =
	    new DBusFprintDevice("net.reactivated.Fprint", path, QDBusConnection::systemBus(), this);

	if (!this->device->isValid()) {
		qCWarning(logFprintDevice) << "Cannot create FprintDevice for" << path;
		return;
	}

	this->deviceProperties.setInterface(this->device);
	this->deviceProperties.updateAllViaGetAll();
}

} // namespace qs::services::fprint
