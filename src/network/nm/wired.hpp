#pragma once

#include <qdbusextratypes.h>
#include <qhash.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../wired.hpp"
#include "active_connection.hpp"
#include "dbus_nm_wired.h"
#include "device.hpp"
#include "network.hpp"
#include "settings.hpp"

namespace qs::network {

// Proxy of a /org/freedesktop/NetworkManager/Device/* object.
// Extends NMDevice to also include members from the org.freedesktop.NetworkManager.Device.Wired interface
// Owns the lifetime of a NMGenericNetwork, and a frontend WiredDevice.
class NMWiredDevice: public NMDevice {
	Q_OBJECT;

public:
	explicit NMWiredDevice(const QString& path, QObject* parent = nullptr);

	[[nodiscard]] bool isValid() const override;
	[[nodiscard]] WiredDevice* frontend() override { return this->mFrontend; };

signals:
	void speedChanged(quint32 speed);

private slots:
	void onSettingsLoaded(NMSettings* settings);
	void onActiveConnectionLoaded(NMActiveConnection* active);

private:
	void initWired();
	void bindFrontend();

	WiredDevice* mFrontend = nullptr;
	NMGenericNetwork* mNetwork = nullptr;

	Q_OBJECT_BINDABLE_PROPERTY(NMWiredDevice, quint32, bSpeed, &NMWiredDevice::speedChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(NMWireless, wiredProperties);
	QS_DBUS_PROPERTY_BINDING(NMWiredDevice, pSpeed, bSpeed, wiredProperties, "Speed");

	DBusNMWiredProxy* wiredProxy = nullptr;
};

}; // namespace qs::network
