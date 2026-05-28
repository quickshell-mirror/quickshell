#include "accesspoint.hpp"

#include <qdbusconnection.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qstring.h>
#include <qtypes.h>

#include "../../core/logcat.hpp"
#include "../../dbus/properties.hpp"
#include "dbus_nm_accesspoint.h"
#include "enums.hpp"

namespace qs::network {
using namespace qs::dbus;

namespace {
QS_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

NMAccessPoint::NMAccessPoint(const QString& path, QObject* parent): QObject(parent) {
	this->proxy = new DBusNMAccessPointProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->proxy->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create DBus interface for access point at" << path;
		return;
	}

	QObject::connect(
	    &this->accessPointProperties,
	    &DBusPropertyGroup::getAllFinished,
	    this,
	    &NMAccessPoint::loaded,
	    Qt::SingleShotConnection
	);

	this->accessPointProperties.setInterface(this->proxy);
	this->accessPointProperties.updateAllViaGetAll();
}

bool NMAccessPoint::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMAccessPoint::address() const { return this->proxy ? this->proxy->service() : QString(); }
QString NMAccessPoint::path() const { return this->proxy ? this->proxy->path() : QString(); }

} // namespace qs::network

namespace qs::dbus {

DBusResult<qs::network::NM80211ApFlags::Enum>
DBusDataTransform<qs::network::NM80211ApFlags::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<qs::network::NM80211ApFlags::Enum>(wire));
}

DBusResult<qs::network::NM80211ApSecurityFlags::Enum>
DBusDataTransform<qs::network::NM80211ApSecurityFlags::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<qs::network::NM80211ApSecurityFlags::Enum>(wire));
}

DBusResult<qs::network::NM80211Mode::Enum>
DBusDataTransform<qs::network::NM80211Mode::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<qs::network::NM80211Mode::Enum>(wire));
}

} // namespace qs::dbus
