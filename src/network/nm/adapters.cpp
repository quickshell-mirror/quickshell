#include "adapters.hpp"

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

NMDeviceAdapter::NMDeviceAdapter(QObject* parent): QObject(parent) {}

void NMDeviceAdapter::init(const QString& path) {
	this->proxy = new DBusNMDeviceProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->proxy->isValid()) {
		qCWarning(logNMDevice) << "Cannot create NMDevice for" << path;
		return;
	}

	this->deviceProperties.setInterface(this->proxy);
	this->deviceProperties.updateAllViaGetAll();
}

bool NMDeviceAdapter::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMDeviceAdapter::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMDeviceAdapter::path() const { return this->proxy ? this->proxy->path() : QString(); }

} // namespace qs::network
