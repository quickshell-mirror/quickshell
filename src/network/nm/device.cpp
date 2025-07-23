#include "device.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "dbus_nm_device.h"

using namespace qs::dbus;

namespace qs::network {

namespace {
Q_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

NMDeviceAdapter::NMDeviceAdapter(const QString& path, QObject* parent): QObject(parent) {
	this->proxy = new DBusNMDeviceProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->proxy->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create DBus interface for device at" << path;
		return;
	}

	this->deviceProperties.setInterface(this->proxy);
	this->deviceProperties.updateAllViaGetAll();
}

void NMDeviceAdapter::disconnect() { this->proxy->Disconnect(); }
bool NMDeviceAdapter::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMDeviceAdapter::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMDeviceAdapter::path() const { return this->proxy ? this->proxy->path() : QString(); }

NetworkDeviceState::Enum NMDeviceState::toNetworkDeviceState(NMDeviceState::Enum state) {
	switch (state) {
	case 0 ... 20: return NetworkDeviceState::Unknown;
	case 30: return NetworkDeviceState::Disconnected;
	case 40 ... 90: return NetworkDeviceState::Connecting;
	case 100: return NetworkDeviceState::Connected;
	case 110 ... 120: return NetworkDeviceState::Disconnecting;
	}
}

} // namespace qs::network

namespace qs::dbus {

DBusResult<qs::network::NMDeviceType::Enum>
DBusDataTransform<qs::network::NMDeviceType::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<qs::network::NMDeviceType::Enum>(wire));
}

DBusResult<qs::network::NMDeviceState::Enum>
DBusDataTransform<qs::network::NMDeviceState::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<qs::network::NMDeviceState::Enum>(wire));
}

} // namespace qs::dbus
