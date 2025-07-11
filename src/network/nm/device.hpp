#pragma once

#include <qcontainerfwd.h>
#include <qdbusextratypes.h>
#include <qdbusservicewatcher.h>
#include <qhash.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "../api.hpp"
#include "dbus_nm_device.h"
#include "wireless.hpp"

namespace qs::network {

class NMDevice: public Device {
	Q_OBJECT;

public:
	explicit NMDevice(QObject* parent = nullptr);
	void init(const QString& path);
	void registerWireless(const QString& path);
	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;

signals:
	void typeChanged();

private:
	// Needed to check if WiFi
	[[nodiscard]] QBindable<quint32> bindableType() const { return &this->bType; };
	Q_PROPERTY(quint32 type READ default NOTIFY typeChanged BINDABLE bindableType);
	Q_OBJECT_BINDABLE_PROPERTY(NMDevice, quint32, bType, &Device::nameChanged);
	QS_DBUS_PROPERTY_BINDING(NMDevice, pType, bType, deviceProperties, "DeviceType");

	QS_DBUS_PROPERTY_BINDING(NMDevice, pName, bName, deviceProperties, "Interface");
	QS_DBUS_PROPERTY_BINDING(NMDevice, pAddress, bAddress, deviceProperties, "HwAddress");

	DBusNMDevice* device = nullptr;
	NMWireless* wireless = nullptr;
};

} // namespace qs::network
