#include "wireless.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "nm/dbus_nm_wireless.h"

using namespace qs::dbus;

namespace qs::network {

namespace {
Q_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

NMWirelessAdapter::NMWirelessAdapter(const QString& path, QObject* parent): QObject(parent) {
	this->proxy = new DBusNMWirelessProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->proxy->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create DBus interface for wireless device at" << path;
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

	QObject::connect(
	    this,
	    &NMWirelessAdapter::activeApChanged,
	    this,
	    &NMWirelessAdapter::onActiveApChanged
	);

	this->wirelessProperties.setInterface(this->proxy);
	this->wirelessProperties.updateAllViaGetAll();

	this->registerAccessPoints();
}

void NMWirelessAdapter::onAccessPointAdded(const QDBusObjectPath& path) {
	this->registerAccessPoint(path.path());
}

void NMWirelessAdapter::onAccessPointRemoved(const QDBusObjectPath& path) {
	auto* ap = mApMap.take(path.path());

	if (!ap) {
		qCDebug(logNetworkManager) << "NetworkManager backend sent removal signal for" << path.path()
		                           << "which is not registered.";
		return;
	}

	removeApFromNetwork(ap);
	delete ap;
}

void NMWirelessAdapter::onActiveApChanged(const QDBusObjectPath& path) {
	// Disconnect all
	for (WifiNetwork* nw: mWifiNetworkMap) {
		if (nw) {
			nw->setConnected(false);
		};
	}

	QByteArray ssid = this->mSsidMap.value(path.path());
	if (ssid != "") {
		WifiNetwork* network = this->mWifiNetworkMap.value(ssid);
		if (network) {
			network->setConnected(true);
			return;
		}
	}
}

void NMWirelessAdapter::registerAccessPoints() {
	auto pending = this->proxy->GetAllAccessPoints();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<QList<QDBusObjectPath>> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager)
			    << "Failed to get all access points: " << reply.error().message();
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
	if (this->mApMap.contains(path)) {
		qCDebug(logNetworkManager) << "Skipping duplicate registration of access point" << path;
		return;
	}

	auto* ap = new NMAccessPointAdapter(path, this);

	if (!ap->isValid()) {
		qCWarning(logNetworkManager) << "Could not create DBus interface for access point at" << path;
		delete ap;
		return;
	}

	this->mApMap.insert(path, ap);
	qCDebug(logNetworkManager) << "Registered access point" << path;

	// AP SSID can change
	QObject::connect(
	    ap,
	    &NMAccessPointAdapter::ssidChanged,
	    this,
	    [this, ap](const QByteArray& ssid) {
		    if (ssid.isEmpty()) { // AP SSID can be hidden
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

	auto* group = mApGroupMap.value(ssid);
	if (!group) {
		auto* network = new WifiNetwork(this);
		group = new NMWifiNetwork(ssid, this);
		network->setSsid(QString::fromUtf8(ssid));

		// NMAccessPointGroup signal -> NetworkWifiNetwork slot
		QObject::connect(
		    group,
		    &NMWifiNetwork::signalStrengthChanged,
		    network,
		    &WifiNetwork::setSignalStrength
		);

		this->mApGroupMap.insert(ssid, group);
		this->mWifiNetworkMap.insert(ssid, network);

		// Sometimes active AP changes before wifi network is registered
		if (this->activeApPath().path() == ap->path()) {
			network->setConnected(true);
		};

		emit this->wifiNetworkAdded(network);
	}

	group->addAccessPoint(ap);
	this->mSsidMap.insert(ap->path(), ssid);
}

void NMWirelessAdapter::removeApFromNetwork(NMAccessPointAdapter* ap) {
	QByteArray ssid = mSsidMap.take(ap->path());
	if (ssid.isEmpty()) return; // AP wasn't previously associated with a wifi network

	auto* group = mApGroupMap.value(ssid);
	group->removeAccessPoint(ap);

	// No access points exist for the wifi network
	if (group->isEmpty()) {
		mApGroupMap.remove(ssid);
		auto* network = mWifiNetworkMap.take(ssid);
		emit wifiNetworkRemoved(network);
		delete network;
		delete group;
	}
}

void NMWirelessAdapter::scan() { this->proxy->RequestScan({}); }

bool NMWirelessAdapter::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMWirelessAdapter::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMWirelessAdapter::path() const { return this->proxy ? this->proxy->path() : QString(); }

} // namespace qs::network
