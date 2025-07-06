
#include "device.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "dbus_device.h"

using namespace qs::dbus;

namespace qs::service::networkmanager {

namespace {
Q_LOGGING_CATEGORY(
    logNetworkManagerDevice,
    "quickshell.service.networkmanager.device",
    QtWarningMsg
);
}

QString NetworkManagerDeviceState::toString(NetworkManagerDeviceState::Enum state) {
	auto metaEnum = QMetaEnum::fromType<NetworkManagerDeviceState::Enum>();
	if (metaEnum.valueToKey(state)) {
		return QString(metaEnum.valueToKey(state));
	}
	return "Invalid state";
}

QString NetworkManagerDeviceType::toString(NetworkManagerDeviceType::Enum type) {
	auto metaEnum = QMetaEnum::fromType<NetworkManagerDeviceType::Enum>();
	if (metaEnum.valueToKey(type)) {
		return QString(metaEnum.valueToKey(type));
	}
	return "Invalid type";
}

NetworkManagerDevice::NetworkManagerDevice(QObject* parent): QObject(parent) {}

void NetworkManagerDevice::init(const QString& path) {
	this->device = new DBusNetworkManagerDevice(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->device->isValid()) {
		qCWarning(logNetworkManagerDevice) << "Cannot create NetworkManagerDevice for" << path;
		return;
	}

	this->deviceProperties.setInterface(this->device);
	this->deviceProperties.updateAllViaGetAll();
}

bool NetworkManagerDevice::isValid() const { return this->device && this->device->isValid(); }
QString NetworkManagerDevice::address() const {
	return this->device ? this->device->service() : QString();
}
QString NetworkManagerDevice::path() const {
	return this->device ? this->device->path() : QString();
}

} // namespace qs::service::networkmanager

namespace qs::dbus {

using namespace qs::service::networkmanager;

DBusResult<NetworkManagerDeviceState::Enum>
DBusDataTransform<NetworkManagerDeviceState::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<NetworkManagerDeviceState::Enum>(wire));
}

DBusResult<NetworkManagerDeviceType::Enum>
DBusDataTransform<NetworkManagerDeviceType::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<NetworkManagerDeviceType::Enum>(wire));
}

} // namespace qs::dbus
