#include "accesspoint.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "nm/dbus_nm_accesspoint.h"

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

namespace qs::network {
using namespace qs::dbus;

namespace {
Q_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

NMAccessPointAdapter::NMAccessPointAdapter(const QString& path, QObject* parent): QObject(parent) {
	this->proxy = new DBusNMAccessPointProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->proxy->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create access point proxy for" << path;
		return;
	}

	QObject::connect(
	    &this->accessPointProperties,
	    &DBusPropertyGroup::getAllFinished,
	    this,
	    [this]() { emit this->ready(); },
	    Qt::SingleShotConnection
	);

	this->accessPointProperties.setInterface(this->proxy);
	this->accessPointProperties.updateAllViaGetAll();
}

bool NMAccessPointAdapter::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMAccessPointAdapter::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMAccessPointAdapter::path() const { return this->proxy ? this->proxy->path() : QString(); }

} // namespace qs::network
