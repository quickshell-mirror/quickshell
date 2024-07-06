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

namespace qs::service::upower {

Q_LOGGING_CATEGORY(logUPowerDevice, "quickshell.service.upower.device", QtWarningMsg);

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

UPowerDevice::UPowerDevice(const QString& path, QObject* parent): QObject(parent) {
	this->device =
	    new DBusUPowerDevice("org.freedesktop.UPower", path, QDBusConnection::systemBus(), this);

	if (!this->device->isValid()) {
		qCWarning(logUPowerDevice) << "Cannot create UPowerDevice for" << path;
		return;
	}

	// clang-format off
	QObject::connect(&this->pType, &AbstractDBusProperty::changed, this, &UPowerDevice::typeChanged);
	QObject::connect(&this->pPowerSupply, &AbstractDBusProperty::changed, this, &UPowerDevice::powerSupplyChanged);
	QObject::connect(&this->pEnergy, &AbstractDBusProperty::changed, this, &UPowerDevice::energyChanged);
	QObject::connect(&this->pEnergyCapacity, &AbstractDBusProperty::changed, this, &UPowerDevice::energyCapacityChanged);
	QObject::connect(&this->pChangeRate, &AbstractDBusProperty::changed, this, &UPowerDevice::changeRateChanged);
	QObject::connect(&this->pTimeToEmpty, &AbstractDBusProperty::changed, this, &UPowerDevice::timeToEmptyChanged);
	QObject::connect(&this->pTimeToFull, &AbstractDBusProperty::changed, this, &UPowerDevice::timeToFullChanged);
	QObject::connect(&this->pPercentage, &AbstractDBusProperty::changed, this, &UPowerDevice::percentageChanged);
	QObject::connect(&this->pIsPresent, &AbstractDBusProperty::changed, this, &UPowerDevice::isPresentChanged);
	QObject::connect(&this->pState, &AbstractDBusProperty::changed, this, &UPowerDevice::stateChanged);
	QObject::connect(&this->pHealthPercentage, &AbstractDBusProperty::changed, this, &UPowerDevice::healthPercentageChanged);
	QObject::connect(&this->pHealthPercentage, &AbstractDBusProperty::changed, this, &UPowerDevice::healthSupportedChanged);
	QObject::connect(&this->pIconName, &AbstractDBusProperty::changed, this, &UPowerDevice::iconNameChanged);
	QObject::connect(&this->pType, &AbstractDBusProperty::changed, this, &UPowerDevice::isLaptopBatteryChanged);
	QObject::connect(&this->pNativePath, &AbstractDBusProperty::changed, this, &UPowerDevice::nativePathChanged);

	QObject::connect(&this->deviceProperties, &DBusPropertyGroup::getAllFinished, this, &UPowerDevice::ready);
	// clang-format on

	this->deviceProperties.setInterface(this->device);
	this->deviceProperties.updateAllViaGetAll();
}

bool UPowerDevice::isValid() const { return this->device->isValid(); }
QString UPowerDevice::address() const { return this->device->service(); }
QString UPowerDevice::path() const { return this->device->path(); }

UPowerDeviceType::Enum UPowerDevice::type() const {
	return static_cast<UPowerDeviceType::Enum>(this->pType.get());
}

bool UPowerDevice::powerSupply() const { return this->pPowerSupply.get(); }
qreal UPowerDevice::energy() const { return this->pEnergy.get(); }
qreal UPowerDevice::energyCapacity() const { return this->pEnergyCapacity.get(); }
qreal UPowerDevice::changeRate() const { return this->pChangeRate.get(); }
qlonglong UPowerDevice::timeToEmpty() const { return this->pTimeToEmpty.get(); }
qlonglong UPowerDevice::timeToFull() const { return this->pTimeToFull.get(); }
qreal UPowerDevice::percentage() const { return this->pPercentage.get(); }
bool UPowerDevice::isPresent() const { return this->pIsPresent.get(); }

UPowerDeviceState::Enum UPowerDevice::state() const {
	return static_cast<UPowerDeviceState::Enum>(this->pState.get());
}

qreal UPowerDevice::healthPercentage() const { return this->pHealthPercentage.get(); }

bool UPowerDevice::healthSupported() const { return this->healthPercentage() != 0; }

QString UPowerDevice::iconName() const { return this->pIconName.get(); }

bool UPowerDevice::isLaptopBattery() const {
	return this->pType.get() == UPowerDeviceType::Battery && this->pPowerSupply.get();
}

QString UPowerDevice::nativePath() const { return this->pNativePath.get(); }

} // namespace qs::service::upower
