#include "connection.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "nm/dbus_nm_connection.h"

using namespace qs::dbus;

namespace qs::network {

namespace {
Q_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

NMConnectionAdapter::NMConnectionAdapter(const QString& path, QObject* parent): QObject(parent) {
	this->proxy = new DBusNMConnectionProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->proxy->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create DBus interface for connection at" << path;
		return;
	}

	this->connectionProperties.setInterface(this->proxy);
	this->connectionProperties.updateAllViaGetAll();
}

bool NMConnectionAdapter::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMConnectionAdapter::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMConnectionAdapter::path() const { return this->proxy ? this->proxy->path() : QString(); }

} // namespace qs::network
