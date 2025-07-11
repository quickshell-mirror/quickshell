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

namespace qs::network {

class NMDevice: public Device {
	Q_OBJECT;

public:
	explicit NMDevice(QObject* parent = nullptr);
	void init(const QString& path);
	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;

private:
	QS_DBUS_PROPERTY_BINDING(NMDevice, pName, bName, deviceProperties, "Interface");
	QS_DBUS_PROPERTY_BINDING(NMDevice, pAddress, bAddress, deviceProperties, "HwAddress");

	DBusNMDevice* device = nullptr;
};

} // namespace qs::network
