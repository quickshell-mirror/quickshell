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

class NMDeviceAdapter: public QObject {
	Q_OBJECT;

public:
	explicit NMDeviceAdapter(QObject* parent = nullptr);
	void init(const QString& path);
	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;
	[[nodiscard]] QString getInterface() { return this->bInterface; };
	[[nodiscard]] QString getHwAddress() { return this->bHwAddress; };

signals:
	void interfaceChanged(const QString& interface);
	void hwAddressChanged(const QString& hwAddress);

private:
	Q_OBJECT_BINDABLE_PROPERTY(NMDeviceAdapter, QString, bInterface, &NMDeviceAdapter::interfaceChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NMDeviceAdapter, QString, bHwAddress, &NMDeviceAdapter::hwAddressChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMDeviceAdapter, deviceProperties);
	QS_DBUS_PROPERTY_BINDING(NMDeviceAdapter, pName, bInterface, deviceProperties, "Interface");
	QS_DBUS_PROPERTY_BINDING(NMDeviceAdapter, pAddress, bHwAddress, deviceProperties, "HwAddress");

	DBusNMDeviceProxy* proxy = nullptr;
};

} // namespace qs::network
