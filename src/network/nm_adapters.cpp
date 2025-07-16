#include "nm_adapters.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtypes.h>

#include "../dbus/properties.hpp"
#include "dbus_nm_accesspoint.h"
#include "dbus_nm_device.h"
#include "dbus_nm_wireless.h"

using namespace qs::dbus;

namespace qs::network {

namespace {
Q_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

// Device

NMDeviceAdapter::NMDeviceAdapter(QObject* parent): QObject(parent) {}

void NMDeviceAdapter::init(const QString& path) {
	this->proxy = new DBusNMDeviceProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->proxy->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create NMDeviceAdapter for" << path;
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
	switch (state) {
	case 0 ... 20: return NetworkDeviceState::Unknown;
	case 30: return NetworkDeviceState::Disconnected;
	case 40 ... 90: return NetworkDeviceState::Connecting;
	case 100: return NetworkDeviceState::Connected;
	case 110 ... 120: return NetworkDeviceState::Disconnecting;
	}
}

// Wireless

NMWirelessAdapter::NMWirelessAdapter(QObject* parent): QObject(parent) {}

void NMWirelessAdapter::init(const QString& path) {
	this->proxy = new DBusNMWirelessProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->proxy->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create NMWirelessAdapter for" << path;
		return;
	}

	QObject::connect(
	    this->proxy,
	    &DBusNMWirelessProxy::AccessPointAdded,
	    this,
	    &NMWirelessAdapter::onAccessPointAdded
	);

	QObject::connect(
	    this->proxy,
	    &DBusNMWirelessProxy::AccessPointRemoved,
	    this,
	    &NMWirelessAdapter::onAccessPointRemoved
	);

	this->wirelessProperties.setInterface(this->proxy);
	this->wirelessProperties.updateAllViaGetAll();

	this->registerAccessPoints();
}

void NMWirelessAdapter::onAccessPointAdded(const QDBusObjectPath& path) {
	this->registerAccessPoint(path.path());
}

void NMWirelessAdapter::onAccessPointRemoved(const QDBusObjectPath& path) {
	auto iter = this->mAPHash.find(path.path());
	if (iter == this->mAPHash.end()) {
		qCWarning(logNetworkManager) << "NMWirelessAdapter sent removal signal for" << path.path()
		                             << "which is not registered.";
	} else {
		auto* ap = iter.value();
		this->mAPHash.erase(iter);
		emit accessPointRemoved(ap);
		qCDebug(logNetworkManager) << "Access point" << path.path() << "removed.";
	}
}

void NMWirelessAdapter::registerAccessPoints() {
	auto pending = this->proxy->GetAllAccessPoints();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<QList<QDBusObjectPath>> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager) << "Failed to get access points: " << reply.error().message();
		} else {
			for (const QDBusObjectPath& devicePath: reply.value()) {
				this->registerAccessPoint(devicePath.path());
			}
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void NMWirelessAdapter::registerAccessPoint(const QString& path) {
	if (this->mAPHash.contains(path)) {
		qCDebug(logNetworkManager) << "Skipping duplicate registration of access point" << path;
		return;
	}

	// Create an access point adapter
	auto* apAdapter = new NMAccessPointAdapter();
	apAdapter->init(path);

	if (!apAdapter->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create NMAccessPointAdapter for" << path;
		delete apAdapter;
		return;
	}
	auto* ap = new NetworkAccessPoint(this);
	apAdapter->setParent(ap);

	// NMAccessPointAdapter signal -> NetworkAccessPoint slot
	QObject::connect(apAdapter, &NMAccessPointAdapter::ssidChanged, ap, &NetworkAccessPoint::setSsid);
	QObject::connect(
	    apAdapter,
	    &NMAccessPointAdapter::signalChanged,
	    ap,
	    &NetworkAccessPoint::setSignal
	);

	this->mAPHash.insert(path, ap);
	emit accessPointAdded(ap);
	qCDebug(logNetworkManager) << "Registered access point" << path;
}

void NMWirelessAdapter::scan() { this->proxy->RequestScan({}); }

bool NMWirelessAdapter::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMWirelessAdapter::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMWirelessAdapter::path() const { return this->proxy ? this->proxy->path() : QString(); }

// Access Point

namespace {
Q_LOGGING_CATEGORY(logNMAccessPoint, "quickshell.network.networkmanager.accesspoint", QtWarningMsg);
}

NMAccessPointAdapter::NMAccessPointAdapter(QObject* parent): QObject(parent) {}

void NMAccessPointAdapter::init(const QString& path) {
	this->proxy = new DBusNMAccessPointProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->proxy->isValid()) {
		qCWarning(logNMAccessPoint) << "Cannot create NMWirelessAdapter for" << path;
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
