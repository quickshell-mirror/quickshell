#pragma once

#include <qdbusextratypes.h>
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
	[[nodiscard]] QBindable<bool> bindableOnBattery() const { return &this->bOnBattery; }

	static UPower* instance();

signals:
	void onBatteryChanged();

private slots:
	void onDeviceAdded(const QDBusObjectPath& path);
	void onDeviceRemoved(const QDBusObjectPath& path);
	void onDeviceReady();

private:
	explicit UPower();

	void init();
	void registerDevices();
	void registerDisplayDevice();
	void registerDevice(const QString& path);

	UPowerDevice mDisplayDevice {this};
	QHash<QString, UPowerDevice*> mDevices;
	ObjectModel<UPowerDevice> readyDevices {this};

	Q_OBJECT_BINDABLE_PROPERTY(UPower, bool, bOnBattery, &UPower::onBatteryChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(UPower, serviceProperties);
	QS_DBUS_PROPERTY_BINDING(UPower, pOnBattery, bOnBattery, serviceProperties, "OnBattery");

	DBusUPowerService* service = nullptr;
};

///! Provides access to the UPower service.
/// An interface to the [UPower daemon], which can be used to
/// view battery and power statistics for your computer and
/// connected devices.
///
/// > [!NOTE] The UPower daemon must be installed to use this service.
///
/// [UPower daemon]: https://upower.freedesktop.org
class UPowerQml: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(UPower);
	QML_SINGLETON;
	// clang-format off
	/// UPower's DisplayDevice for your system. Cannot be null,
	/// but might not be initialized (check @@UPowerDevice.ready if you need to know).
	///
	/// This is an aggregate device and not a physical one, meaning you will not find it in @@devices.
	/// It is typically the device that is used for displaying information in desktop environments.
	Q_PROPERTY(qs::service::upower::UPowerDevice* displayDevice READ displayDevice CONSTANT);
	/// All connected UPower devices.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::service::upower::UPowerDevice>*);
	Q_PROPERTY(UntypedObjectModel* devices READ devices CONSTANT);
	/// If the system is currently running on battery power, or discharging.
	Q_PROPERTY(bool onBattery READ default NOTIFY onBatteryChanged BINDABLE bindableOnBattery);
	// clang-format on

public:
	explicit UPowerQml(QObject* parent = nullptr);

	[[nodiscard]] UPowerDevice* displayDevice();
	[[nodiscard]] ObjectModel<UPowerDevice>* devices();

	[[nodiscard]] static QBindable<bool> bindableOnBattery() {
		return UPower::instance()->bindableOnBattery();
	}

signals:
	void onBatteryChanged();
};

} // namespace qs::service::upower
