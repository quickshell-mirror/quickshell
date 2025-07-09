
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
    logNMDevice,
    "quickshell.service.networkmanager.device",
    QtWarningMsg
);
}

QString NMDeviceState::toString(NMDeviceState::Enum state) {
	auto metaEnum = QMetaEnum::fromType<NMDeviceState::Enum>();
	if (metaEnum.valueToKey(state)) {
		return QString(metaEnum.valueToKey(state));
	}
	return "Invalid state";
}

QString NMDeviceType::toString(NMDeviceType::Enum type) {
	auto metaEnum = QMetaEnum::fromType<NMDeviceType::Enum>();
	if (metaEnum.valueToKey(type)) {
		return QString(metaEnum.valueToKey(type));
	}
	return "Invalid type";
}

NMDevice::NMDevice(QObject* parent): QObject(parent) {}

void NMDevice::init(const QString& path) {
	this->device = new DBusNMDevice(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->device->isValid()) {
		qCWarning(logNMDevice) << "Cannot create NMDevice for" << path;
		return;
	}

	this->deviceProperties.setInterface(this->device);
	this->deviceProperties.updateAllViaGetAll();
}

bool NMDevice::isValid() const { return this->device && this->device->isValid(); }
QString NMDevice::address() const {
	return this->device ? this->device->service() : QString();
}
QString NMDevice::path() const {
	return this->device ? this->device->path() : QString();
}

} // namespace qs::service::networkmanager

namespace qs::dbus {

using namespace qs::service::networkmanager;

DBusResult<NMDeviceState::Enum>
DBusDataTransform<NMDeviceState::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<NMDeviceState::Enum>(wire));
}

DBusResult<NMDeviceType::Enum>
DBusDataTransform<NMDeviceType::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<NMDeviceType::Enum>(wire));
}

} // namespace qs::dbus
