#include "device.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtypes.h>

#include "../../core/logcat.hpp"
#include "../../dbus/properties.hpp"
#include "dbus_device.h"

using namespace qs::dbus;

namespace qs::service::upower {

namespace {
QS_LOGGING_CATEGORY(logUPowerDevice, "quickshell.service.upower.device", QtWarningMsg);
}

QString UPowerDeviceState::toString(UPowerDeviceState::Enum status) {
	switch (status) {
	case UPowerDeviceState::Unknown: return "Unknown";
	case UPowerDeviceState::Charging: return "Charging";
	case UPowerDeviceState::Discharging: return "Discharging";
	case UPowerDeviceState::Empty: return "Empty";
	case UPowerDeviceState::FullyCharged: return "Fully Charged";
	case UPowerDeviceState::PendingCharge: return "Pending Charge";
	case UPowerDeviceState::PendingDischarge: return "Pending Discharge";
	default: return "Invalid Status";
	}
}

QString UPowerDeviceType::toString(UPowerDeviceType::Enum type) {
	switch (type) {
	case UPowerDeviceType::Unknown: return "Unknown";
	case UPowerDeviceType::LinePower: return "Line Power";
	case UPowerDeviceType::Battery: return "Battery";
	case UPowerDeviceType::Ups: return "Ups";
	case UPowerDeviceType::Monitor: return "Monitor";
	case UPowerDeviceType::Mouse: return "Mouse";
	case UPowerDeviceType::Keyboard: return "Keyboard";
	case UPowerDeviceType::Pda: return "Pda";
	case UPowerDeviceType::Phone: return "Phone";
	case UPowerDeviceType::MediaPlayer: return "Media Player";
	case UPowerDeviceType::Tablet: return "Tablet";
	case UPowerDeviceType::Computer: return "Computer";
	case UPowerDeviceType::GamingInput: return "Gaming Input";
	case UPowerDeviceType::Pen: return "Pen";
	case UPowerDeviceType::Touchpad: return "Touchpad";
	case UPowerDeviceType::Modem: return "Modem";
	case UPowerDeviceType::Network: return "Network";
	case UPowerDeviceType::Headset: return "Headset";
	case UPowerDeviceType::Speakers: return "Speakers";
	case UPowerDeviceType::Headphones: return "Headphones";
	case UPowerDeviceType::Video: return "Video";
	case UPowerDeviceType::OtherAudio: return "Other Audio";
	case UPowerDeviceType::RemoteControl: return "Remote Control";
	case UPowerDeviceType::Printer: return "Printer";
	case UPowerDeviceType::Scanner: return "Scanner";
	case UPowerDeviceType::Camera: return "Camera";
	case UPowerDeviceType::Wearable: return "Wearable";
	case UPowerDeviceType::Toy: return "Toy";
	case UPowerDeviceType::BluetoothGeneric: return "Bluetooth Generic";
	default: return "Invalid Type";
	}
}

UPowerDevice::UPowerDevice(QObject* parent): QObject(parent) {
	this->bIsLaptopBattery.setBinding([this]() {
		return this->bType == UPowerDeviceType::Battery && this->bPowerSupply;
	});

	this->bHealthSupported.setBinding([this]() { return this->bHealthPercentage != 0; });
}

void UPowerDevice::init(const QString& path) {
	this->device =
	    new DBusUPowerDevice("org.freedesktop.UPower", path, QDBusConnection::systemBus(), this);

	if (!this->device->isValid()) {
		qCWarning(logUPowerDevice) << "Cannot create UPowerDevice for" << path;
		return;
	}

	QObject::connect(
	    &this->deviceProperties,
	    &DBusPropertyGroup::getAllFinished,
	    this,
	    &UPowerDevice::onGetAllFinished
	);

	this->deviceProperties.setInterface(this->device);
	this->deviceProperties.updateAllViaGetAll();
}

bool UPowerDevice::isValid() const { return this->device && this->device->isValid(); }
QString UPowerDevice::address() const { return this->device ? this->device->service() : QString(); }
QString UPowerDevice::path() const { return this->device ? this->device->path() : QString(); }

void UPowerDevice::onGetAllFinished() {
	qCDebug(logUPowerDevice) << "UPowerDevice" << device->path() << "ready.";
	this->bReady = true;
}

} // namespace qs::service::upower

namespace qs::dbus {

using namespace qs::service::upower;

DBusResult<qreal> DBusDataTransform<PowerPercentage>::fromWire(qreal wire) {
	return DBusResult(wire * 0.01);
}

DBusResult<UPowerDeviceState::Enum>
DBusDataTransform<UPowerDeviceState::Enum>::fromWire(quint32 wire) {
	if (wire >= UPowerDeviceState::Unknown && wire <= UPowerDeviceState::PendingDischarge) {
		return DBusResult(static_cast<UPowerDeviceState::Enum>(wire));
	}

	return DBusResult<UPowerDeviceState::Enum>(
	    QDBusError(QDBusError::InvalidArgs, QString("Invalid UPowerDeviceState: %1").arg(wire))
	);
}

DBusResult<UPowerDeviceType::Enum> DBusDataTransform<UPowerDeviceType::Enum>::fromWire(quint32 wire
) {
	if (wire >= UPowerDeviceType::Unknown && wire <= UPowerDeviceType::BluetoothGeneric) {
		return DBusResult(static_cast<UPowerDeviceType::Enum>(wire));
	}

	return DBusResult<UPowerDeviceType::Enum>(
	    QDBusError(QDBusError::InvalidArgs, QString("Invalid UPowerDeviceType: %1").arg(wire))
	);
}

} // namespace qs::dbus
