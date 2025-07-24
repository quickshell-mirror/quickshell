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
	enum Enum : quint8 {
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

	Q_INVOKABLE static QString toString(qs::service::upower::UPowerDeviceState::Enum status);
};

///! Type of a UPower device.
/// See @@UPowerDevice.type.
class UPowerDeviceType: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
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

	Q_INVOKABLE static QString toString(qs::service::upower::UPowerDeviceType::Enum type);
};

struct PowerPercentage;

} // namespace qs::service::upower

namespace qs::dbus {

template <>
struct DBusDataTransform<qs::service::upower::UPowerDeviceState::Enum> {
	using Wire = quint32;
	using Data = qs::service::upower::UPowerDeviceState::Enum;
	static DBusResult<Data> fromWire(Wire wire);
};

template <>
struct DBusDataTransform<qs::service::upower::UPowerDeviceType::Enum> {
	using Wire = quint32;
	using Data = qs::service::upower::UPowerDeviceType::Enum;
	static DBusResult<Data> fromWire(Wire wire);
};

template <>
struct DBusDataTransform<qs::service::upower::PowerPercentage> {
	using Wire = qreal;
	using Data = qreal;
	static DBusResult<Data> fromWire(Wire wire);
};

} // namespace qs::dbus

namespace qs::service::upower {

///! A device exposed through the UPower system service.
class UPowerDevice: public QObject {
	Q_OBJECT;
	// clang-format off
	/// The type of device.
	Q_PROPERTY(qs::service::upower::UPowerDeviceType::Enum type READ default NOTIFY typeChanged BINDABLE bindableType);
	/// If the device is a power supply for your computer and can provide charge.
	Q_PROPERTY(bool powerSupply READ default NOTIFY powerSupplyChanged BINDABLE bindablePowerSupply);
	/// Current energy level of the device in watt-hours.
	Q_PROPERTY(qreal energy READ default NOTIFY energyChanged BINDABLE bindableEnergy);
	/// Maximum energy capacity of the device in watt-hours
	Q_PROPERTY(qreal energyCapacity READ default NOTIFY energyCapacityChanged BINDABLE bindableEnergyCapacity);
	/// Rate of energy change in watts (positive when charging, negative when discharging).
	Q_PROPERTY(qreal changeRate READ default NOTIFY changeRateChanged BINDABLE bindableChangeRate);
	/// Estimated time until the device is fully discharged, in seconds.
	///
	/// Will be set to `0` if charging.
	Q_PROPERTY(qreal timeToEmpty READ default NOTIFY timeToEmptyChanged BINDABLE bindableTimeToEmpty);
	/// Estimated time until the device is fully charged, in seconds.
	///
	/// Will be set to `0` if discharging.
	Q_PROPERTY(qreal timeToFull READ default NOTIFY timeToFullChanged BINDABLE bindableTimeToFull);
	/// Current charge level as a percentage.
	///
	/// This would be equivalent to @@energy / @@energyCapacity.
	Q_PROPERTY(qreal percentage READ default NOTIFY percentageChanged BINDABLE bindablePercentage);
	/// If the power source is present in the bay or slot, useful for hot-removable batteries.
	///
	/// If the device `type` is not `Battery`, then the property will be invalid.
	Q_PROPERTY(bool isPresent READ default NOTIFY isPresentChanged BINDABLE bindableIsPresent);
	/// Current state of the device.
	Q_PROPERTY(qs::service::upower::UPowerDeviceState::Enum state READ default NOTIFY stateChanged BINDABLE bindableState);
	/// Health of the device as a percentage of its original health.
	Q_PROPERTY(qreal healthPercentage READ default NOTIFY healthPercentageChanged BINDABLE bindableHealthPercentage);
	Q_PROPERTY(bool healthSupported READ default NOTIFY healthSupportedChanged BINDABLE bindableHealthSupported);
	/// Name of the icon representing the current state of the device, or an empty string if not provided.
	Q_PROPERTY(QString iconName READ default NOTIFY iconNameChanged BINDABLE bindableIconName);
	/// If the device is a laptop battery or not. Use this to check if your device is a valid battery.
	///
	/// This will be equivalent to @@type == Battery && @@powerSupply == true.
	Q_PROPERTY(bool isLaptopBattery READ default NOTIFY isLaptopBatteryChanged BINDABLE bindableIsLaptopBattery);
	/// Native path of the device specific to your OS.
	Q_PROPERTY(QString nativePath READ default NOTIFY nativePathChanged BINDABLE bindableNativePath);
	/// Model name of the device. Unlikely to be useful for internal devices.
	Q_PROPERTY(QString model READ default NOTIFY modelChanged BINDABLE bindableModel);
	/// If device statistics have been queried for this device yet.
  /// This will be true for all devices returned from @@UPower.devices, but not the default
  /// device, which may be returned before it is ready to avoid returning null.
	Q_PROPERTY(bool ready READ default NOTIFY readyChanged BINDABLE bindableReady);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("UPowerDevices can only be acquired from UPower");

public:
	explicit UPowerDevice(QObject* parent = nullptr);

	void init(const QString& path);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] QString path() const;

	[[nodiscard]] QBindable<UPowerDeviceType::Enum> bindableType() const { return &this->bType; }
	[[nodiscard]] QBindable<bool> bindablePowerSupply() const { return &this->bPowerSupply; }
	[[nodiscard]] QBindable<qreal> bindableEnergy() const { return &this->bEnergy; }
	[[nodiscard]] QBindable<qreal> bindableEnergyCapacity() const { return &this->bEnergyCapacity; }
	[[nodiscard]] QBindable<qreal> bindableChangeRate() const { return &this->bChangeRate; }
	[[nodiscard]] QBindable<qlonglong> bindableTimeToEmpty() const { return &this->bTimeToEmpty; }
	[[nodiscard]] QBindable<qlonglong> bindableTimeToFull() const { return &this->bTimeToFull; }
	[[nodiscard]] QBindable<qreal> bindablePercentage() const { return &this->bPercentage; }
	[[nodiscard]] QBindable<bool> bindableIsPresent() const { return &this->bIsPresent; }
	[[nodiscard]] QBindable<UPowerDeviceState::Enum> bindableState() const { return &this->bState; }
	[[nodiscard]] QBindable<qreal> bindableHealthPercentage() const {
		return &this->bHealthPercentage;
	}
	[[nodiscard]] QBindable<bool> bindableHealthSupported() const { return &this->bHealthSupported; }
	[[nodiscard]] QBindable<QString> bindableIconName() const { return &this->bIconName; }
	[[nodiscard]] QBindable<bool> bindableIsLaptopBattery() const { return &this->bIsLaptopBattery; }
	[[nodiscard]] QBindable<QString> bindableNativePath() const { return &this->bNativePath; }
	[[nodiscard]] QBindable<QString> bindableModel() const { return &this->bModel; }
	[[nodiscard]] QBindable<bool> bindableReady() const { return &this->bReady; }

signals:
	QSDOC_HIDE void readyChanged();

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
	void modelChanged();

private slots:
	void onGetAllFinished();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(UPowerDevice, UPowerDeviceType::Enum, bType, &UPowerDevice::typeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(UPowerDevice, bool, bPowerSupply, &UPowerDevice::powerSupplyChanged);
	Q_OBJECT_BINDABLE_PROPERTY(UPowerDevice, qreal, bEnergy, &UPowerDevice::energyChanged);
	Q_OBJECT_BINDABLE_PROPERTY(UPowerDevice, qreal, bEnergyCapacity, &UPowerDevice::energyCapacityChanged);
	Q_OBJECT_BINDABLE_PROPERTY(UPowerDevice, qreal, bChangeRate, &UPowerDevice::changeRateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(UPowerDevice, qlonglong, bTimeToEmpty, &UPowerDevice::timeToEmptyChanged);
	Q_OBJECT_BINDABLE_PROPERTY(UPowerDevice, qlonglong, bTimeToFull, &UPowerDevice::timeToFullChanged);
	Q_OBJECT_BINDABLE_PROPERTY(UPowerDevice, qreal, bPercentage, &UPowerDevice::percentageChanged);
	Q_OBJECT_BINDABLE_PROPERTY(UPowerDevice, bool, bIsPresent, &UPowerDevice::isPresentChanged);
	Q_OBJECT_BINDABLE_PROPERTY(UPowerDevice, UPowerDeviceState::Enum, bState, &UPowerDevice::stateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(UPowerDevice, qreal, bHealthPercentage, &UPowerDevice::healthPercentageChanged);
	Q_OBJECT_BINDABLE_PROPERTY(UPowerDevice, bool, bHealthSupported, &UPowerDevice::healthSupportedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(UPowerDevice, QString, bIconName, &UPowerDevice::iconNameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(UPowerDevice, bool, bIsLaptopBattery, &UPowerDevice::isLaptopBatteryChanged);
	Q_OBJECT_BINDABLE_PROPERTY(UPowerDevice, QString, bNativePath, &UPowerDevice::nativePathChanged);
	Q_OBJECT_BINDABLE_PROPERTY(UPowerDevice, QString, bModel, &UPowerDevice::modelChanged);
	Q_OBJECT_BINDABLE_PROPERTY(UPowerDevice, bool, bReady, &UPowerDevice::readyChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(UPowerDevice, deviceProperties);
	QS_DBUS_PROPERTY_BINDING(UPowerDevice, pType, bType, deviceProperties, "Type");
	QS_DBUS_PROPERTY_BINDING(UPowerDevice, pPowerSupply, bPowerSupply, deviceProperties, "PowerSupply");
	QS_DBUS_PROPERTY_BINDING(UPowerDevice, pEnergy, bEnergy, deviceProperties, "Energy");
	QS_DBUS_PROPERTY_BINDING(UPowerDevice, pEnergyCapacity, bEnergyCapacity, deviceProperties, "EnergyFull");
	QS_DBUS_PROPERTY_BINDING(UPowerDevice, pChangeRate, bChangeRate, deviceProperties, "EnergyRate");
	QS_DBUS_PROPERTY_BINDING(UPowerDevice, pTimeToEmpty, bTimeToEmpty, deviceProperties, "TimeToEmpty");
	QS_DBUS_PROPERTY_BINDING(UPowerDevice, pTimeToFull, bTimeToFull, deviceProperties, "TimeToFull");
	QS_DBUS_PROPERTY_BINDING(UPowerDevice, PowerPercentage, pPercentage, bPercentage, deviceProperties, "Percentage", true);
	QS_DBUS_PROPERTY_BINDING(UPowerDevice, pIsPresent, bIsPresent, deviceProperties, "IsPresent");
	QS_DBUS_PROPERTY_BINDING(UPowerDevice, pState, bState, deviceProperties, "State");
	QS_DBUS_PROPERTY_BINDING(UPowerDevice, pHealthPercentage, bHealthPercentage, deviceProperties, "Capacity");
	QS_DBUS_PROPERTY_BINDING(UPowerDevice, pIconName, bIconName, deviceProperties, "IconName");
	QS_DBUS_PROPERTY_BINDING(UPowerDevice, pNativePath, bNativePath, deviceProperties, "NativePath");
	QS_DBUS_PROPERTY_BINDING(UPowerDevice, pModel, bModel, deviceProperties, "Model");
	// clang-format on

	DBusUPowerDevice* device = nullptr;
};

} // namespace qs::service::upower
