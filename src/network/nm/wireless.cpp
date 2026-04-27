#include "wireless.hpp"

#include <qcontainerfwd.h>
#include <qdatetime.h>
#include <qdbusconnection.h>
#include <qdbusextratypes.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "../../core/logcat.hpp"
#include "../../dbus/properties.hpp"
#include "../enums.hpp"
#include "../wifi.hpp"
#include "accesspoint.hpp"
#include "active_connection.hpp"
#include "dbus_nm_wireless.h"
#include "dbus_types.hpp"
#include "device.hpp"
#include "enums.hpp"
#include "network.hpp"
#include "settings.hpp"
#include "utils.hpp"

namespace qs::network {
using namespace qs::dbus;

namespace {
QS_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

NMWirelessDevice::NMWirelessDevice(const QString& path, QObject* parent)
    : NMDevice(path, parent)
    , mScanTimer(this) {
	this->wirelessProxy = new DBusNMWirelessProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	if (!this->wirelessProxy->isValid()) {
		qCWarning(logNetworkManager) << "Cannot create DBus interface for wireless device at" << path;
		return;
	}

	QObject::connect(
	    &this->wirelessProperties,
	    &DBusPropertyGroup::getAllFinished,
	    this,
	    &NMWirelessDevice::initWireless,
	    Qt::SingleShotConnection
	);

	QObject::connect(&this->mScanTimer, &QTimer::timeout, this, &NMWirelessDevice::onScanTimeout);
	this->mScanTimer.setSingleShot(true);

	this->wirelessProperties.setInterface(this->wirelessProxy);
	this->wirelessProperties.updateAllViaGetAll();

	// Register and bind the frontend WifiDevice.
	this->mFrontend = new WifiDevice(this);
	this->bindFrontend();
}

void NMWirelessDevice::initWireless() {
	// clang-format off
	QObject::connect(this->wirelessProxy, &DBusNMWirelessProxy::AccessPointAdded, this, &NMWirelessDevice::onAccessPointAdded);
	QObject::connect(this->wirelessProxy, &DBusNMWirelessProxy::AccessPointRemoved, this, &NMWirelessDevice::onAccessPointRemoved);
	QObject::connect(this, &NMWirelessDevice::accessPointLoaded, this, &NMWirelessDevice::onAccessPointLoaded);
	QObject::connect(this, &NMWirelessDevice::settingsLoaded, this, &NMWirelessDevice::onSettingsLoaded);
	QObject::connect(this, &NMWirelessDevice::activeConnectionLoaded, this, &NMWirelessDevice::onActiveConnectionLoaded);
	QObject::connect(this, &NMWirelessDevice::scanningChanged, this, &NMWirelessDevice::onScanningChanged);
	// clang-format on

	this->registerAccessPoints();
	emit this->loaded();
}

void NMWirelessDevice::onAccessPointAdded(const QDBusObjectPath& path) {
	this->registerAccessPoint(path.path());
}

void NMWirelessDevice::onAccessPointRemoved(const QDBusObjectPath& path) {
	auto* ap = this->mAccessPoints.take(path.path());
	if (!ap) {
		qCDebug(logNetworkManager) << "Sent removal signal for" << path.path()
		                           << "which is not registered.";
		return;
	}
	qCDebug(logNetworkManager) << "Access point removed:" << path.path();
	delete ap;
}

void NMWirelessDevice::onAccessPointLoaded(NMAccessPoint* ap) {
	const QString ssid = ap->ssid();
	if (!ssid.isEmpty()) {
		auto mode = ap->mode();
		if (mode == NM80211Mode::Infra) {
			auto* net = this->mNetworks.value(ssid);
			if (!net) net = this->registerNetwork(ssid);
			net->addAccessPoint(ap);
		}
	}
}

void NMWirelessDevice::onSettingsLoaded(NMSettings* settings) {
	const NMSettingsMap& map = settings->map();
	// Filter connections that aren't wireless or have missing settings
	if (map["connection"]["id"].toString().isEmpty() || map["connection"]["uuid"].toString().isEmpty()
	    || !map.contains("802-11-wireless") || map["802-11-wireless"]["ssid"].toString().isEmpty())
	{
		return;
	}

	const auto ssid = map["802-11-wireless"]["ssid"].toString();
	const auto mode = map["802-11-wireless"]["mode"].toString();

	if (mode == "infrastructure") {
		auto* net = this->mNetworks.value(ssid);
		if (!net) net = this->registerNetwork(ssid);
		net->addSettings(settings);

		// Check for active connections that loaded before their respective connection settings
		auto* active = this->activeConnection();
		if (active && settings->path() == active->connection().path()) {
			net->addActiveConnection(active);
		}
	}
	// TODO: Create hotspots when mode == "ap"
}

void NMWirelessDevice::onActiveConnectionLoaded(NMActiveConnection* active) {
	// Find an existing network with connection settings that matches the active
	const QString activeConnPath = active->connection().path();
	for (const auto& net: this->mNetworks.values()) {
		for (auto* settings: net->settings()) {
			if (activeConnPath == settings->path()) {
				net->addActiveConnection(active);
				return;
			}
		}
	}
}

void NMWirelessDevice::onScanTimeout() {
	const QDateTime now = QDateTime::currentDateTime();
	const QDateTime lastScan = this->bLastScan;
	const QDateTime lastScanRequest = this->mLastScanRequest;

	if (lastScan.isValid() && lastScan.msecsTo(now) < this->mScanIntervalMs) {
		// Rate limit if backend last scan property updated within the interval
		auto diff = static_cast<int>(this->mScanIntervalMs - lastScan.msecsTo(now));
		this->mScanTimer.start(diff);
	} else if (lastScanRequest.isValid() && lastScanRequest.msecsTo(now) < this->mScanIntervalMs) {
		// Rate limit if frontend changes scanner state within the interval
		auto diff = static_cast<int>(this->mScanIntervalMs - lastScanRequest.msecsTo(now));
		this->mScanTimer.start(diff);
	} else {
		this->wirelessProxy->RequestScan({});
		this->mLastScanRequest = now;
		this->mScanTimer.start(this->mScanIntervalMs);
	}
}

void NMWirelessDevice::onScanningChanged(bool scanning) {
	scanning ? this->onScanTimeout() : this->mScanTimer.stop();
}

void NMWirelessDevice::registerAccessPoints() {
	auto pending = this->wirelessProxy->GetAllAccessPoints();
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

void NMWirelessDevice::registerAccessPoint(const QString& path) {
	if (this->mAccessPoints.contains(path)) {
		qCDebug(logNetworkManager) << "Skipping duplicate registration of access point" << path;
		return;
	}

	auto* ap = new NMAccessPoint(path, this);

	if (!ap->isValid()) {
		qCWarning(logNetworkManager) << "Ignoring invalid registration of" << path;
		delete ap;
		return;
	}

	qCDebug(logNetworkManager) << "Access point added:" << path;
	this->mAccessPoints.insert(path, ap);
	QObject::connect(
	    ap,
	    &NMAccessPoint::loaded,
	    this,
	    [this, ap]() { emit this->accessPointLoaded(ap); },
	    Qt::SingleShotConnection
	);
	ap->bindableSecurity().setBinding([this, ap]() {
		return findBestWirelessSecurity(
		    this->bCapabilities,
		    ap->mode() == NM80211Mode::Adhoc,
		    ap->flags(),
		    ap->wpaFlags(),
		    ap->rsnFlags()
		);
	});
}

NMWirelessNetwork* NMWirelessDevice::registerNetwork(const QString& ssid) {
	auto* net = new NMWirelessNetwork(ssid, this->frontend(), this);

	this->NMDevice::bindNetwork(net);
	auto visible = [this, net]() {
		return this->bScanning || net->state() == NMConnectionState::Activated || net->known();
	};
	net->bindableVisible().setBinding(visible);
	net->bindableActiveApPath().setBinding([this]() { return this->activeApPath().path(); });
	QObject::connect(net, &NMWirelessNetwork::disappeared, this, &NMWirelessDevice::removeNetwork);

	this->mNetworks.insert(ssid, net);
	qCDebug(logNetworkManager) << "Registered network for SSID" << ssid;
	return net;
}

void NMWirelessDevice::removeNetwork() {
	auto* net = qobject_cast<NMWirelessNetwork*>(this->sender());
	if (this->mNetworks.take(net->ssid())) {
		if (net->visible()) emit this->networkRemoved(net->frontend());
		delete net;
	};
}

void NMWirelessDevice::bindFrontend() {
	auto* frontend = this->mFrontend;
	this->NMDevice::bindFrontend(frontend);
	auto translateMode = [this]() {
		switch (this->mode()) {
		case NM80211Mode::Unknown: return WifiDeviceMode::Unknown;
		case NM80211Mode::Adhoc: return WifiDeviceMode::AdHoc;
		case NM80211Mode::Infra: return WifiDeviceMode::Station;
		case NM80211Mode::Ap: return WifiDeviceMode::AccessPoint;
		case NM80211Mode::Mesh: return WifiDeviceMode::Mesh;
		}
	};
	frontend->bindableMode().setBinding(translateMode);
	this->bindableScanning().setBinding([frontend]() { return frontend->scannerEnabled(); });
}

bool NMWirelessDevice::isValid() const {
	return this->NMDevice::isValid() && (this->wirelessProxy && this->wirelessProxy->isValid());
}

} // namespace qs::network

namespace qs::dbus {

DBusResult<qs::network::NMWirelessCapabilities::Enum>
DBusDataTransform<qs::network::NMWirelessCapabilities::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<qs::network::NMWirelessCapabilities::Enum>(wire));
}

DBusResult<QDateTime> DBusDataTransform<QDateTime>::fromWire(qint64 wire) {
	return DBusResult(qs::network::clockBootTimeToDateTime(wire));
}

} // namespace qs::dbus
