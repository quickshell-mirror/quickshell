#include "wired.hpp"

#include <qdbusconnection.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qstring.h>
#include <qtmetamacros.h>

#include "../../core/logcat.hpp"
#include "../../dbus/properties.hpp"
#include "../wired.hpp"
#include "connection.hpp"
#include "dbus_nm_wired.h"
#include "dbus_types.hpp"
#include "device.hpp"
#include "enums.hpp"
#include "network.hpp"

namespace qs::network {
using namespace qs::dbus;

namespace {
QS_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

NMWiredDevice::NMWiredDevice(const QString& path, QObject* parent): NMDevice(path, parent) {
	this->wiredProxy = new DBusNMWiredProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->wiredProxy->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create DBus interface for wired device at" << path;
		return;
	}

	// Wait to create the NMGenericNetwork/Network
	// until the dbus properties load because the network requires the interface name.
	QObject::connect(
	    &this->wiredProperties,
	    &DBusPropertyGroup::getAllFinished,
	    this,
	    &NMWiredDevice::initWired,
	    Qt::SingleShotConnection
	);

	this->wiredProperties.setInterface(this->wiredProxy);
	this->wiredProperties.updateAllViaGetAll();

	// Register and bind the frontend WiredDevice.
	this->mFrontend = new WiredDevice(this);
	this->bindFrontend();
};

void NMWiredDevice::initWired() {
	// Register the NMGenericNetwork and bind the frontend Network.
	// For wired networking, there is only one Network and it should exist for the lifetime of the device.
	auto* net = new NMGenericNetwork(QString(), this->frontend(), this);
	net->frontend()->bindableName().setBinding([this]() { return this->interface(); });

	auto visible = [this]() {
		return (this->interfaceFlags() & NMDeviceInterfaceFlags::Carrier) != 0;
	};
	net->bindableVisible().setBinding(visible);
	this->NMDevice::bindNetwork(net);
	this->mNetwork = net;

	emit this->loaded();
}

void NMWiredDevice::bindFrontend() {
	auto* frontend = this->mFrontend;
	this->NMDevice::bindFrontend(frontend);
	frontend->bindableLinkSpeed().setBinding([this]() { return this->bSpeed.value(); });
	frontend->bindableHasLink().setBinding([this]() {
		return (this->interfaceFlags() & NMDeviceInterfaceFlags::Carrier) != 0;
	});
}

bool NMWiredDevice::isConnectionCompatible(const NMSettings& settings) const {
	return this->NMDevice::isConnectionCompatible(settings)
	    && settings.value("connection").value("type").toString() == "802-3-ethernet";
}

NMNetwork* NMWiredDevice::networkForConnection(NMConnection* conn) {
	return this->isConnectionCompatible(conn->settings()) ? this->mNetwork : nullptr;
}

bool NMWiredDevice::isValid() const {
	return this->NMDevice::isValid() && (this->wiredProxy && this->wiredProxy->isValid());
}

} // namespace qs::network
