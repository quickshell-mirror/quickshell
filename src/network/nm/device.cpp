#include "device.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtypes.h>

#include "../dbus/properties.hpp"
#include "dbus_nm_device.h"

using namespace qs::dbus;

namespace qs::network {

namespace {
Q_LOGGING_CATEGORY(logNMDevice, "quickshell.network.networkmanager.device", QtWarningMsg);
}

NMDevice::NMDevice(QObject* parent): Device(parent) {}

void NMDevice::init(const QString& path) {
	this->device =
	    new DBusNMDevice("org.freedesktop.NetworkManager", path, QDBusConnection::systemBus(), this);

	if (!this->device->isValid()) {
		qCWarning(logNMDevice) << "Cannot create NMDevice for" << path;
		return;
	}

	this->deviceProperties.setInterface(this->device);
	this->deviceProperties.updateAllViaGetAll();
}

bool NMDevice::isValid() const { return this->device && this->device->isValid(); }
QString NMDevice::address() const { return this->device ? this->device->service() : QString(); }
QString NMDevice::path() const { return this->device ? this->device->path() : QString(); }

} // namespace qs::network
