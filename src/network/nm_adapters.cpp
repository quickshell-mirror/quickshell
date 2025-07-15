#include "nm_adapters.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtypes.h>

#include "../dbus/properties.hpp"
#include "dbus_nm_device.h"

using namespace qs::dbus;

namespace qs::network {

namespace {
Q_LOGGING_CATEGORY(logNMDevice, "quickshell.network.networkmanager.device", QtWarningMsg);
}

NMDeviceAdapter::NMDeviceAdapter(QObject* parent): QObject(parent) {}

void NMDeviceAdapter::init(const QString& path) {
	this->proxy = new DBusNMDeviceProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->proxy->isValid()) {
		qCWarning(logNMDevice) << "Cannot create NMDeviceAdapter for" << path;
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

NetworkDeviceState::Enum NMDeviceState::translate(NMDeviceState::Enum state) {
	switch(state) {
		case 0 ... 20: return NetworkDeviceState::Unknown;
		case 30: return NetworkDeviceState::Disconnected;
		case 40 ... 90: return NetworkDeviceState::Connecting;
		case 100: return NetworkDeviceState::Connected;
		case 110 ... 120: return NetworkDeviceState::Disconnecting;
	}
}

NMWirelessAdapter::NMWirelessAdapter(QObject* parent): QObject(parent) {}

void NMWirelessAdapter::init(const QString& path) {
	this->proxy = new DBusNMWirelessProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->proxy->isValid()) {
		qCWarning(logNMDevice) << "Cannot create NMWirelessAdapter for" << path;
		return;
	}

	this->wirelessProperties.setInterface(this->proxy);
	this->wirelessProperties.updateAllViaGetAll();
}

void NMWirelessAdapter::scan() { this->proxy->RequestScan(); }
bool NMWirelessAdapter::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMWirelessAdapter::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMWirelessAdapter::path() const { return this->proxy ? this->proxy->path() : QString(); }
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
