#include "active_connection.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qdbuspendingcall.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qstring.h>
#include <qtypes.h>

#include "../../core/logcat.hpp"
#include "../../dbus/properties.hpp"
#include "../enums.hpp"
#include "dbus_nm_active_connection.h"
#include "enums.hpp"

namespace qs::network {
using namespace qs::dbus;

namespace {
QS_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

NMActiveConnection::NMActiveConnection(const QString& path, QObject* parent): QObject(parent) {
	this->proxy = new DBusNMActiveConnectionProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->proxy->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create DBus interface for connection at" << path;
		return;
	}

	// clang-format off
	QObject::connect(&this->activeConnectionProperties, &DBusPropertyGroup::getAllFinished, this, &NMActiveConnection::loaded, Qt::SingleShotConnection);
	QObject::connect(this->proxy, &DBusNMActiveConnectionProxy::StateChanged, this, &NMActiveConnection::onStateChanged);
	// clang-format on

	this->activeConnectionProperties.setInterface(this->proxy);
	this->activeConnectionProperties.updateAllViaGetAll();
}

void NMActiveConnection::onStateChanged(quint32 /*state*/, quint32 reason) {
	auto enumReason = static_cast<NMNetworkStateReason::Enum>(reason);
	if (this->bStateReason == enumReason) return;
	this->bStateReason = enumReason;
}

bool NMActiveConnection::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMActiveConnection::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMActiveConnection::path() const { return this->proxy ? this->proxy->path() : QString(); }

} // namespace qs::network

namespace qs::dbus {

DBusResult<qs::network::NMConnectionState::Enum>
DBusDataTransform<qs::network::NMConnectionState::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<qs::network::NMConnectionState::Enum>(wire));
}

} // namespace qs::dbus
