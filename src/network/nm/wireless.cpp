#include "wireless.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtypes.h>

#include "../dbus/properties.hpp"
#include "dbus_nm_wireless.h"

using namespace qs::dbus;

namespace qs::network {

namespace {
Q_LOGGING_CATEGORY(logNMDevice, "quickshell.network.networkmanager.wireless", QtWarningMsg);
}

NMWireless::NMWireless(QObject* parent): Wireless(parent) {}

void NMWireless::init(const QString& path) {
	this->wireless =
	    new DBusNMWireless("org.freedesktop.NetworkManager", path, QDBusConnection::systemBus(), this);

	if (!this->wireless->isValid()) {
		qCWarning(logNMDevice) << "Cannot create NMDevice for" << path;
		return;
	}

	this->wirelessProperties.setInterface(this->wireless);
	this->wirelessProperties.updateAllViaGetAll();
}

bool NMWireless::isValid() const { return this->wireless && this->wireless->isValid(); }
QString NMWireless::address() const { return this->wireless ? this->wireless->service() : QString(); }
QString NMWireless::path() const { return this->wireless ? this->wireless->path() : QString(); }

} // namespace qs::network
