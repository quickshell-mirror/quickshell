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
		qCWarning(logNetworkManager) << "Cannot create dbus proxy for" << path;
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
		qCWarning(logNetworkManager) << "Cannot create dbus proxy for" << path;
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
	auto* ap = mPathToApHash.take(path.path());

	if (!ap) {
		qCDebug(logNetworkManager) << "NetworkManager backend sent removal signal for" << path.path()
		                           << "which is not registered.";
		return;
	}

	removeApFromNetwork(ap);
	delete ap;
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
	if (this->mPathToApHash.contains(path)) {
		qCDebug(logNetworkManager) << "Skipping duplicate registration of access point" << path;
		return;
	}

	auto* ap = new NMAccessPointAdapter(this);
	ap->init(path);

	if (!ap->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create access point for" << path;
		delete ap;
		return;
	}

	this->mPathToApHash.insert(path, ap);
	qCDebug(logNetworkManager) << "Registered access point" << path;

	QObject::connect(
	    ap,
	    &NMAccessPointAdapter::ssidChanged,
	    this,
	    [this, ap](const QByteArray& ssid) {
		    if (ssid.isEmpty()) {
			    this->removeApFromNetwork(ap);
		    } else {
			    this->addApToNetwork(ap, ssid);
		    }
	    }
	);
}

void NMWirelessAdapter::addApToNetwork(NMAccessPointAdapter* ap, const QByteArray& ssid) {
	// Remove AP from old network
	removeApFromNetwork(ap);

	auto* group = mSsidToApGroupHash.value(ssid);
	if (!group) {
		// Create a new AP group and wifi network
		group = new NMAccessPointGroup(ssid, this);
		auto* network = new NetworkWifiNetwork(this);
		network->setSsid(ssid);

		// NMAccessPointGroup signal -> NetworkWifiNetwork slot
		QObject::connect(
		    group,
		    &NMAccessPointGroup::maxSignalChanged,
		    network,
		    &NetworkWifiNetwork::setSignal
		);

		this->mSsidToApGroupHash.insert(ssid, group);
		this->mSsidToNetworkHash.insert(ssid, network);
		emit this->wifiNetworkAdded(network);
		qCDebug(logNetworkManager) << "Registered wifi network" << ssid;
	}

	group->addAccessPoint(ap);
	this->mPathToSsidHash.insert(ap->path(), ssid);
}

void NMWirelessAdapter::removeApFromNetwork(NMAccessPointAdapter* ap) {
	QByteArray ssid = mPathToSsidHash.take(ap->path());
	if (ssid.isEmpty()) return; // AP wasn't in any network

	auto* group = mSsidToApGroupHash.value(ssid);
	group->removeAccessPoint(ap);
	if (group->isEmpty()) {
		mSsidToApGroupHash.remove(ssid);
		auto* network = mSsidToNetworkHash.take(ssid);
		emit wifiNetworkRemoved(network);
		delete network;
		delete group;
		qCDebug(logNetworkManager) << "Deleted wifi network" << ssid;
	}
}

void NMWirelessAdapter::scan() { this->proxy->RequestScan({}); }

bool NMWirelessAdapter::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMWirelessAdapter::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMWirelessAdapter::path() const { return this->proxy ? this->proxy->path() : QString(); }

// Access Point

NMAccessPointAdapter::NMAccessPointAdapter(QObject* parent): QObject(parent) {}

void NMAccessPointAdapter::init(const QString& path) {
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

// Access Point Group

NMAccessPointGroup::NMAccessPointGroup(const QByteArray& ssid, QObject* parent)
    : QObject(parent)
    , mSsid(ssid) {}

void NMAccessPointGroup::updateMaxSignal() {
	quint8 max = 0;
	for (auto* ap: mAccessPoints) {
		max = qMax(max, ap->getSignal());
	}
	if (this->bMaxSignal != max) {
		this->bMaxSignal = max;
	}
}

void NMAccessPointGroup::addAccessPoint(NMAccessPointAdapter* ap) {
	if (this->mAccessPoints.contains(ap)) return;

	this->mAccessPoints.append(ap);
	QObject::connect(
	    ap,
	    &NMAccessPointAdapter::signalChanged,
	    this,
	    &NMAccessPointGroup::updateMaxSignal
	);
	this->updateMaxSignal();
}

void NMAccessPointGroup::removeAccessPoint(NMAccessPointAdapter* ap) {
	if (mAccessPoints.removeOne(ap)) {
		QObject::disconnect(
		    ap,
		    &NMAccessPointAdapter::signalChanged,
		    this,
		    &NMAccessPointGroup::updateMaxSignal
		);
		this->updateMaxSignal();
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
