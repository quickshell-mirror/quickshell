#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/doc.hpp"
#include "../../dbus/properties.hpp"
#include "dbus_device.h"

namespace qs::service::upower {

///! Power state of a UPower device.
/// See @@UPowerDevice.state.
class UPowerDeviceState: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum {
		Unknown = 0,
		Charging = 1,
		Discharging = 2,
		Empty = 3,
		FullyCharged = 4,
		/// The device is waiting to be charged after it was plugged in.
		PendingCharge = 5,
		/// The device is waiting to be discharged after being unplugged.
		PendingDischarge = 6,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(UPowerDeviceState::Enum status);
};

///! Type of a UPower device.
/// See @@UPowerDevice.type.
class UPowerDeviceType: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum {
		Unknown = 0,
		LinePower = 1,
		Battery = 2,
		Ups = 3,
		Monitor = 4,
		Mouse = 5,
		Keyboard = 6,
		Pda = 7,
		Phone = 8,
		MediaPlayer = 9,
		Tablet = 10,
		Computer = 11,
		GamingInput = 12,
		Pen = 13,
		Touchpad = 14,
		Modem = 15,
		Network = 16,
		Headset = 17,
		Speakers = 18,
		Headphones = 19,
		Video = 20,
		OtherAudio = 21,
		RemoteControl = 22,
		Printer = 23,
		Scanner = 24,
		Camera = 25,
		Wearable = 26,
		Toy = 27,
		BluetoothGeneric = 28,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(UPowerDeviceType::Enum type);
};

///! A device exposed through the UPower system service.
class UPowerDevice: public QObject {
	Q_OBJECT;
	// clang-format off
	/// The type of device.
	Q_PROPERTY(qs::service::upower::UPowerDeviceType::Enum type READ type NOTIFY typeChanged);
	/// If the device is a power supply for your computer and can provide charge.
	Q_PROPERTY(bool powerSupply READ powerSupply NOTIFY powerSupplyChanged);
	/// Current energy level of the device in watt-hours.
	Q_PROPERTY(qreal energy READ energy NOTIFY energyChanged);
	/// Maximum energy capacity of the device in watt-hours
	Q_PROPERTY(qreal energyCapacity READ energyCapacity NOTIFY energyCapacityChanged);
	/// Rate of energy change in watts (positive when charging, negative when discharging).
	Q_PROPERTY(qreal changeRate READ changeRate NOTIFY changeRateChanged);
	/// Estimated time until the device is fully discharged, in seconds.
	///
	/// Will be set to `0` if charging.
	Q_PROPERTY(qreal timeToEmpty READ timeToEmpty NOTIFY timeToEmptyChanged);
	/// Estimated time until the device is fully charged, in seconds.
	///
	/// Will be set to `0` if discharging.
	Q_PROPERTY(qreal timeToFull READ timeToFull NOTIFY timeToFullChanged);
	/// Current charge level as a percentage.
	///
	/// This would be equivalent to @@energy / @@energyCapacity.
	Q_PROPERTY(qreal percentage READ percentage NOTIFY percentageChanged);
	/// If the power source is present in the bay or slot, useful for hot-removable batteries.
	///
	/// If the device `type` is not `Battery`, then the property will be invalid.
	Q_PROPERTY(bool isPresent READ isPresent NOTIFY isPresentChanged);
	/// Current state of the device.
	Q_PROPERTY(qs::service::upower::UPowerDeviceState::Enum state READ state NOTIFY stateChanged);
	/// Health of the device as a percentage of its original health.
	Q_PROPERTY(qreal healthPercentage READ healthPercentage NOTIFY healthPercentageChanged);
	Q_PROPERTY(bool healthSupported READ healthSupported NOTIFY healthSupportedChanged);
	/// Name of the icon representing the current state of the device, or an empty string if not provided.
	Q_PROPERTY(QString iconName READ iconName NOTIFY iconNameChanged);
	/// If the device is a laptop battery or not. Use this to check if your device is a valid battery.
	///
	/// This will be equivalent to @@type == Battery && @@powerSupply == true.
 	Q_PROPERTY(bool isLaptopBattery READ isLaptopBattery NOTIFY isLaptopBatteryChanged);
	/// Native path of the device specific to your OS.
	Q_PROPERTY(QString nativePath READ nativePath NOTIFY nativePathChanged);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("UPowerDevices can only be acquired from UPower");

public:
	explicit UPowerDevice(const QString& path, QObject* parent = nullptr);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] QString path() const;

	[[nodiscard]] UPowerDeviceType::Enum type() const;
	[[nodiscard]] bool powerSupply() const;
	[[nodiscard]] qreal energy() const;
	[[nodiscard]] qreal energyCapacity() const;
	[[nodiscard]] qreal changeRate() const;
	[[nodiscard]] qlonglong timeToEmpty() const;
	[[nodiscard]] qlonglong timeToFull() const;
	[[nodiscard]] qreal percentage() const;
	[[nodiscard]] bool isPresent() const;
	[[nodiscard]] UPowerDeviceState::Enum state() const;
	[[nodiscard]] qreal healthPercentage() const;
	[[nodiscard]] bool healthSupported() const;
	[[nodiscard]] QString iconName() const;
	[[nodiscard]] bool isLaptopBattery() const;
	[[nodiscard]] QString nativePath() const;

signals:
	QSDOC_HIDE void ready();

	void typeChanged();
	void powerSupplyChanged();
	void energyChanged();
	void energyCapacityChanged();
	void changeRateChanged();
	void timeToEmptyChanged();
	void timeToFullChanged();
	void percentageChanged();
	void isPresentChanged();
	void stateChanged();
	void healthPercentageChanged();
	void healthSupportedChanged();
	void iconNameChanged();
	void isLaptopBatteryChanged();
	void nativePathChanged();

private:
	dbus::DBusPropertyGroup deviceProperties;
	dbus::DBusProperty<quint32> pType {this->deviceProperties, "Type"};
	dbus::DBusProperty<bool> pPowerSupply {this->deviceProperties, "PowerSupply"};
	dbus::DBusProperty<qreal> pEnergy {this->deviceProperties, "Energy"};
	dbus::DBusProperty<qreal> pEnergyCapacity {this->deviceProperties, "EnergyFull"};
	dbus::DBusProperty<qreal> pChangeRate {this->deviceProperties, "EnergyRate"};
	dbus::DBusProperty<qlonglong> pTimeToEmpty {this->deviceProperties, "TimeToEmpty"};
	dbus::DBusProperty<qlonglong> pTimeToFull {this->deviceProperties, "TimeToFull"};
	dbus::DBusProperty<qreal> pPercentage {this->deviceProperties, "Percentage"};
	dbus::DBusProperty<bool> pIsPresent {this->deviceProperties, "IsPresent"};
	dbus::DBusProperty<quint32> pState {this->deviceProperties, "State"};
	dbus::DBusProperty<qreal> pHealthPercentage {this->deviceProperties, "Capacity"};
	dbus::DBusProperty<QString> pIconName {this->deviceProperties, "IconName"};
	dbus::DBusProperty<QString> pNativePath {this->deviceProperties, "NativePath"};

	DBusUPowerDevice* device = nullptr;
};

} // namespace qs::service::upower
