#include "wireless.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtypes.h>

#include "dbus_types.hpp"
#include "nm/enums.hpp"
#include "utils.hpp"

namespace qs::dbus {

DBusResult<qs::network::NMWirelessCapabilities::Enum>
DBusDataTransform<qs::network::NMWirelessCapabilities::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<qs::network::NMWirelessCapabilities::Enum>(wire));
}

} // namespace qs::dbus

namespace qs::network {
using namespace qs::dbus;

namespace {
Q_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

NMWirelessNetwork::NMWirelessNetwork(
    NMWirelessCapabilities::Enum caps,
    QByteArray ssid,
    QObject* parent
)
    : QObject(parent)
    , mDeviceCapabilities(caps)
    , mSsid(std::move(ssid)) {}

void NMWirelessNetwork::updateSignalStrength() {
	quint8 max = 0;
	NMAccessPointAdapter* maxAP = nullptr;
	for (auto* ap: mAccessPoints) {
		quint8 signal = ap->signalStrength();
		if (signal > max) {
			max = signal;
			maxAP = ap;
		}
	}
	if (this->bMaxSignal != max) {
		this->bMaxSignal = max;
	}
	if (maxAP && this->mReferenceAP != maxAP) {
		this->mReferenceAP = maxAP;
		emit referenceAccessPointChanged(maxAP);
	}
}

void NMWirelessNetwork::addAccessPoint(NMAccessPointAdapter* ap) {
	if (this->mAccessPoints.contains(ap)) {
		qCWarning(logNetworkManager) << "Access point" << ap->path() << "was already in WifiNetwork"
		                             << this;
		return;
	}

	this->mAccessPoints.append(ap);
	if (this->mReferenceAP == nullptr) {
		this->mReferenceAP = ap;
		emit referenceAccessPointChanged(ap);
	}
	QObject::connect(
	    ap,
	    &NMAccessPointAdapter::signalStrengthChanged,
	    this,
	    &NMWirelessNetwork::updateSignalStrength
	);
	this->updateSignalStrength();
}

void NMWirelessNetwork::removeAccessPoint(NMAccessPointAdapter* ap) {
	if (mAccessPoints.removeOne(ap)) {
		QObject::disconnect(ap, nullptr, this, nullptr);
		this->updateSignalStrength();
	}
}

NMWirelessSecurityType::Enum NMWirelessNetwork::findBestSecurity() {
	if (this->mReferenceAP) {
		auto* ap = this->mReferenceAP;
		return findBestWirelessSecurity(
		    this->mDeviceCapabilities,
		    true,
		    ap->mode() == NM80211Mode::Adhoc,
		    ap->flags(),
		    ap->wpaFlags(),
		    ap->rsnFlags()
		);
	}
	return NMWirelessSecurityType::Unknown;
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

	// We need the properties available to successfully register access points.
	// Their security type is dependent on this adapters capabilities.
	QObject::connect(
	    &this->wirelessProperties,
	    &DBusPropertyGroup::getAllFinished,
	    this,
	    [this]() { this->init(); },
	    Qt::SingleShotConnection
	);

	this->wirelessProperties.setInterface(this->proxy);
	this->wirelessProperties.updateAllViaGetAll();
}

void NMWirelessAdapter::init() {
	// clang-format off
	QObject::connect(this->proxy, &DBusNMWirelessProxy::AccessPointAdded, this, &NMWirelessAdapter::onAccessPointAdded);
	QObject::connect(this->proxy, &DBusNMWirelessProxy::AccessPointRemoved, this, &NMWirelessAdapter::onAccessPointRemoved);
	// clang-format on
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

	QObject::disconnect(ap, nullptr, nullptr, nullptr);
	removeApFromNetwork(ap);
	delete ap;
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

	// Wait for properties
	QObject::connect(
	    ap,
	    &NMAccessPointAdapter::ready,
	    this,
	    [this, ap]() { this->onAccessPointReady(ap); },
	    Qt::SingleShotConnection
	);
}

void NMWirelessAdapter::onAccessPointReady(NMAccessPointAdapter* ap) {
	if (!ap->ssid().isEmpty()) {
		this->addApToNetwork(ap, ap->ssid());
	}

	// The access points SSID can change/hide
	QObject::connect(
	    ap,
	    &NMAccessPointAdapter::ssidChanged,
	    this,
	    [this, ap](const QByteArray& ssid) {
		    if (ssid.isEmpty()) {
			    this->removeApFromNetwork(ap);
		    } else {
			    qCDebug(logNetworkManager)
			        << "Access point" << ap->path() << "changed to ssid" << ap->ssid();
			    this->addApToNetwork(ap, ssid);
		    }
	    }
	);
}

void NMWirelessAdapter::addApToNetwork(NMAccessPointAdapter* ap, const QByteArray& ssid) {
	// Remove AP from previous network
	removeApFromNetwork(ap);

	auto* network = mNetworkMap.value(ssid);
	if (!network) {
		network = new NMWirelessNetwork(this->capabilities(), ssid, this);
		network->addAccessPoint(ap);
		this->mNetworkMap.insert(ssid, network);
		this->mSsidMap.insert(ap->path(), ssid);
		emit this->networkAdded(network);
	} else {
		network->addAccessPoint(ap);
		this->mSsidMap.insert(ap->path(), ssid);
	}
}

void NMWirelessAdapter::removeApFromNetwork(NMAccessPointAdapter* ap) {
	QByteArray ssid = mSsidMap.take(ap->path());
	if (ssid.isEmpty()) return; // AP wasn't previously associated with a network

	auto* network = mNetworkMap.value(ssid);
	network->removeAccessPoint(ap);

	// No access points remain for the wifi network
	if (network->isEmpty()) {
		mNetworkMap.remove(ssid);
		emit networkRemoved(network);
		delete network;
	}
}

void NMWirelessAdapter::scan() { this->proxy->RequestScan({}); }
bool NMWirelessAdapter::isValid() const { return this->proxy && this->proxy->isValid(); }
QString NMWirelessAdapter::address() const {
	return this->proxy ? this->proxy->service() : QString();
}
QString NMWirelessAdapter::path() const { return this->proxy ? this->proxy->path() : QString(); }

NMWirelessManager::NMWirelessManager(QObject* parent): QObject(parent) {};

void NMWirelessManager::connectionLoaded(NMConnectionSettingsAdapter* connection) {
	ConnectionSettingsMap settings = connection->settings();
	if (settings["connection"]["id"].toString().isEmpty()
	    || settings["connection"]["uuid"].toString().isEmpty()
	    || !settings.contains("802-11-wireless")
	    || settings["802-11-wireless"]["mode"].toString() == "ap") // Can't be devices own hotspot
	{
		return;
	}

	// Check if this connection already supplies an item
	QString uuid = settings["connection"]["uuid"].toString();
	if (this->mConnectionMap.contains(uuid)) {
		return;
		qCWarning(logNetworkManager) << "Wireless manager out of sync";
	};
	mConnectionMap.insert(uuid, connection);

	// Is there an item with only a network supplying it?
	QString ssid = settings["802-11-wireless"]["ssid"].toString();
	for (WifiNetwork* item: mSsidToItem.values(ssid)) {
		if (!item->property("saved").toBool()) {
			// Supply existing item
			item->setKnown(true);
			item->setNmSecurity(securityFromConnectionSettings(settings));
			mUuidToItem.insert(uuid, item);
			return;
		}
	}

	// Create a new item and supply it with only the connection
	auto* item = new WifiNetwork(this);
	item->setSsid(ssid);
	item->setKnown(true);
	item->setNmSecurity(securityFromConnectionSettings(settings));
	mUuidToItem.insert(uuid, item);
	emit wifiNetworkAdded(item);

	// Is there a network that can supply this item?
	auto* network = mNetworkMap.value(ssid);
	if (network) {
		item->setSignalStrength(network->signalStrength());
		QObject::connect(
		    network,
		    &NMWirelessNetwork::signalStrengthChanged,
		    item,
		    &WifiNetwork::setSignalStrength
		);
		mSsidToItem.insert(ssid, item);
	}
};

void NMWirelessManager::connectionRemoved(NMConnectionSettingsAdapter* connection) {
	ConnectionSettingsMap settings = connection->settings();

	// Check if this connection supplies an item
	QString uuid = settings["connection"]["uuid"].toString();
	if (!this->mConnectionMap.contains(uuid)) {
		return;
		qCWarning(logNetworkManager) << "Wireless manager out of sync";
	}
	mConnectionMap.take(uuid);

	// Does the item this connectoin supplies also have a network supplying it?
	auto* item = mUuidToItem.take(uuid);
	QString ssid = settings["802-11-wireless"]["ssid"].toString();
	if (mSsidToItem.contains(ssid)) {
		// Is the network supplying multiple items?
		if (mSsidToItem.count(ssid) != 1) {
			mSsidToItem.remove(ssid, item);
			emit wifiNetworkRemoved(item);
			delete item;
		} else {
			// Keep the item and supply only from network
			item->setKnown(false);
			auto* network = mNetworkMap.value(ssid);
			item->setNmSecurity(network->findBestSecurity());
		}
		return;
	}

	// Delete item
	emit wifiNetworkRemoved(item);
	delete item;
};

void NMWirelessManager::networkAdded(NMWirelessNetwork* network) {
	QString ssid = network->ssid();
	if (this->mNetworkMap.contains(ssid)) {
		return;
	};
	mNetworkMap.insert(ssid, network);

	// Find all existing items with a connection supplier
	quint8 count = 0;
	for (WifiNetwork* item: mUuidToItem.values()) {
		if (item->property("ssid").toString() == ssid) {
			count += 1;
			item->setSignalStrength(network->signalStrength());
			QObject::connect(
			    network,
			    &NMWirelessNetwork::signalStrengthChanged,
			    item,
			    &WifiNetwork::setSignalStrength
			);
			mSsidToItem.insert(ssid, item);
		}
	}

	// Was there none?
	if (count != 0) {
		return;
	}

	// Create an item and supply it with the network
	auto* item = new WifiNetwork(this);
	item->setSsid(ssid);
	item->setSignalStrength(network->signalStrength());
	QObject::connect(
	    network,
	    &NMWirelessNetwork::signalStrengthChanged,
	    item,
	    &WifiNetwork::setSignalStrength
	);
	item->setKnown(false);
	item->setNmSecurity(network->findBestSecurity());
	mSsidToItem.insert(ssid, item);
	emit this->wifiNetworkAdded(item);
};

void NMWirelessManager::networkRemoved(NMWirelessNetwork* network) {
	QString ssid = network->ssid();
	if (!this->mNetworkMap.contains(ssid)) {
		return;
	}
	mNetworkMap.take(ssid);

	// Remove network supplies from all supplied items
	for (WifiNetwork* item: mSsidToItem.values(ssid)) {
		// Was this item also supplied by a connection?
		QString uuid = mUuidToItem.key(item);
		if (!uuid.isEmpty()) {
			// Supply only with connection
			QObject::disconnect(network, nullptr, item, nullptr);
			auto* conn = mConnectionMap.value(uuid);
			item->setNmSecurity(securityFromConnectionSettings(conn->settings()));
		} else {
			// Delete item
			emit this->wifiNetworkRemoved(item);
			delete item;
		}
	};
	mSsidToItem.remove(ssid);
};

} // namespace qs::network
