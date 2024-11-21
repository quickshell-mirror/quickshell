#pragma once

#include <qdbusservicewatcher.h>
#include <qhash.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>

#include "../../core/doc.hpp"
#include "../../core/model.hpp"
#include "../../dbus/properties.hpp"
#include "dbus_service.h"
#include "device.hpp"

namespace qs::service::upower {

class UPower: public QObject {
	Q_OBJECT;

public:
	[[nodiscard]] UPowerDevice* displayDevice();
	[[nodiscard]] ObjectModel<UPowerDevice>* devices();
	QS_BINDABLE_GETTER(bool, bOnBattery, onBattery, bindableOnBattery);

	static UPower* instance();

signals:
	void displayDeviceChanged();
	void onBatteryChanged();

private slots:
	void onDeviceReady();
	void onDeviceDestroyed(QObject* object);

private:
	explicit UPower();

	void init();
	void registerExisting();
	void registerDevice(const QString& path);

	UPowerDevice* mDisplayDevice = nullptr;
	QHash<QString, UPowerDevice*> mDevices;
	ObjectModel<UPowerDevice> readyDevices {this};

	Q_OBJECT_BINDABLE_PROPERTY(UPower, bool, bOnBattery, &UPower::onBatteryChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(UPower, serviceProperties);
	QS_DBUS_PROPERTY_BINDING(UPower, pOnBattery, bOnBattery, serviceProperties, "OnBattery");

	DBusUPowerService* service = nullptr;
};

class UPowerQml: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(UPower);
	QML_SINGLETON;
	// clang-format off
	/// UPower's DisplayDevice for your system. Can be `null`.
	///
	/// This is an aggregate device and not a physical one, meaning you will not find it in @@devices.
	/// It is typically the device that is used for displaying information in desktop environments.
	Q_PROPERTY(qs::service::upower::UPowerDevice* displayDevice READ displayDevice NOTIFY displayDeviceChanged);
	/// All connected UPower devices.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::service::upower::UPowerDevice>*);
	Q_PROPERTY(UntypedObjectModel* devices READ devices CONSTANT);
	/// If the system is currently running on battery power, or discharging.
	Q_PROPERTY(bool onBattery READ onBattery NOTIFY onBatteryChanged BINDABLE bindableOnBattery);
	// clang-format on

public:
	explicit UPowerQml(QObject* parent = nullptr);

	[[nodiscard]] UPowerDevice* displayDevice();
	[[nodiscard]] ObjectModel<UPowerDevice>* devices();
	[[nodiscard]] static bool onBattery() { return UPower::instance()->onBattery(); }

	[[nodiscard]] static QBindable<bool> bindableOnBattery() {
		return UPower::instance()->bindableOnBattery();
	}

signals:
	void displayDeviceChanged();
	void onBatteryChanged();
};

} // namespace qs::service::upower
