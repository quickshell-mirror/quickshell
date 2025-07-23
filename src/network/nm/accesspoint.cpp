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

using namespace qs::dbus;

namespace qs::network {

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

	this->accessPointProperties.setInterface(this->proxy);
	this->accessPointProperties.updateAllViaGetAll();
}

bool NMAccessPointAdapter::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMAccessPointAdapter::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMAccessPointAdapter::path() const { return this->proxy ? this->proxy->path() : QString(); }

NMAccessPointGroup::NMAccessPointGroup(QByteArray ssid, QObject* parent)
    : QObject(parent)
    , mSsid(std::move(ssid)) {}

void NMAccessPointGroup::updateSignalStrength() {
	quint8 max = 0;
	for (auto* ap: mAccessPoints) {
		max = qMax(max, ap->getSignal());
	}
	if (this->bMaxSignal != max) {
		this->bMaxSignal = max;
	}
}

void NMAccessPointGroup::addAccessPoint(NMAccessPointAdapter* ap) {
	if (this->mAccessPoints.contains(ap)) {
		qCWarning(logNetworkManager) << "Access point" << ap->path() << "was already in AP group";
		return;
	}

	this->mAccessPoints.append(ap);
	QObject::connect(
	    ap,
	    &NMAccessPointAdapter::signalStrengthChanged,
	    this,
	    &NMAccessPointGroup::updateSignalStrength
	);
	this->updateSignalStrength();
}

void NMAccessPointGroup::removeAccessPoint(NMAccessPointAdapter* ap) {
	if (mAccessPoints.removeOne(ap)) {
		QObject::disconnect(ap, nullptr, this, nullptr);
		this->updateSignalStrength();
	}
}

} // namespace qs::network
