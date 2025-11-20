#include "wireless.hpp"
#include <utility>

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

#include "../../core/logcat.hpp"
#include "../../dbus/properties.hpp"
#include "../wifi.hpp"
#include "accesspoint.hpp"
#include "connection.hpp"
#include "dbus_types.hpp"
#include "device.hpp"
#include "enums.hpp"
#include "nm/dbus_nm_wireless.h"
#include "utils.hpp"

namespace qs::network {
using namespace qs::dbus;

namespace {
QS_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

NMWirelessNetwork::NMWirelessNetwork(QString ssid, QObject* parent)
    : QObject(parent)
    , mSsid(std::move(ssid))
    , bKnown(false)
    , bSecurity(WifiSecurityType::Open)
    , bReason(NMConnectionStateReason::None)
    , bState(NMConnectionState::Deactivated) {}

void NMWirelessNetwork::updateReferenceConnection() {
	// If the network has no connections, the reference is nullptr.
	if (this->mConnections.isEmpty()) {
		this->mReferenceConn = nullptr;
		this->updateReferenceAp(); // Set security back to reference AP.
		return;
	};

	// If the network has an active connection, use it as the reference.
	if (this->mActiveConnection) {
		auto* ref = this->mConnections.value(this->mActiveConnection->connection().path());
		if (ref) {
			this->mReferenceConn = ref;
			return;
		}
	}

	// Otherwise, choose the connection with the strongest security settings.
	auto selectedSecurity = WifiSecurityType::Unknown;
	NMConnectionSettings* selectedConn = nullptr;
	for (auto* conn: this->mConnections) {
		auto security = securityFromConnectionSettings(conn->settings());
		if (selectedSecurity >= security) {
			selectedSecurity = security;
			selectedConn = conn;
		}
	}
	if (selectedConn && this->mReferenceConn != selectedConn) {
		this->mReferenceConn = selectedConn;
		this->bSecurity = selectedSecurity;
	}
}

void NMWirelessNetwork::updateReferenceAp() {
	quint8 selectedStrength = 0;
	NMAccessPoint* selectedAp = nullptr;

	for (auto* ap: this->mAccessPoints.values()) {
		// Always prefer the active AP if found.
		if (ap->path() == this->bActiveApPath) {
			selectedStrength = ap->signalStrength();
			selectedAp = ap;
			break;
		}

		// Otherwise, track the strongest signal.
		if (selectedStrength <= ap->signalStrength()) {
			selectedStrength = ap->signalStrength();
			selectedAp = ap;
		}
	}

	if (selectedStrength != this->bSignalStrength) {
		this->bSignalStrength = selectedStrength;
	}

	if (selectedAp && this->mReferenceAp != selectedAp) {
		// Update reference AP.
		this->mReferenceAp = selectedAp;
		// Reference AP is used for security when there's no connection settings.
		if (this->mReferenceConn) return;
		auto security = findBestWirelessSecurity(
		    this->bCaps,
		    this->mReferenceAp->mode() == NM80211Mode::Adhoc,
		    this->mReferenceAp->flags(),
		    this->mReferenceAp->wpaFlags(),
		    this->mReferenceAp->rsnFlags()
		);
		this->bSecurity = security;
	}
}

void NMWirelessNetwork::addAccessPoint(NMAccessPoint* ap) {
	if (this->mAccessPoints.contains(ap->path())) return;
	this->mAccessPoints.insert(ap->path(), ap);
	// clang-format off
	QObject::connect(ap, &NMAccessPoint::signalStrengthChanged, this, &NMWirelessNetwork::updateReferenceAp);
	QObject::connect(ap, &NMAccessPoint::disappeared, this, &NMWirelessNetwork::removeAccessPoint);
	// clang-format on
	this->updateReferenceAp();
};

void NMWirelessNetwork::removeAccessPoint() {
	auto* ap = qobject_cast<NMAccessPoint*>(this->sender());
	if (this->mAccessPoints.take(ap->path())) {
		if (this->mAccessPoints.isEmpty()) {
			emit this->disappeared();
		} else {
			QObject::disconnect(ap, nullptr, this, nullptr);
			this->updateReferenceAp();
		}
	}
};

void NMWirelessNetwork::addConnectionSettings(NMConnectionSettings* conn) {
	if (this->mConnections.contains(conn->path())) return;
	this->mConnections.insert(conn->path(), conn);
	QObject::connect(
	    conn,
	    &NMConnectionSettings::disappeared,
	    this,
	    &NMWirelessNetwork::removeConnectionSettings
	);
	this->updateReferenceConnection();
	this->bKnown = true;
};

void NMWirelessNetwork::removeConnectionSettings() {
	auto* conn = qobject_cast<NMConnectionSettings*>(this->sender());
	if (this->mConnections.take(conn->path())) {
		QObject::disconnect(conn, nullptr, this, nullptr);
		this->updateReferenceConnection();
		if (this->mConnections.isEmpty()) {
			this->bKnown = false;
		}
	}
};

void NMWirelessNetwork::addActiveConnection(NMActiveConnection* active) {
	if (this->mActiveConnection) return;
	this->mActiveConnection = active;
	this->bState.setBinding([active]() { return active->state(); });
	this->bReason.setBinding([active]() { return active->stateReason(); });
	QObject::connect(
	    active,
	    &NMActiveConnection::disappeared,
	    this,
	    &NMWirelessNetwork::removeActiveConnection
	);
};

void NMWirelessNetwork::removeActiveConnection() {
	auto* active = qobject_cast<NMActiveConnection*>(this->sender());
	if (this->mActiveConnection && this->mActiveConnection == active) {
		QObject::disconnect(active, nullptr, this, nullptr);
		this->bState = NMConnectionState::Deactivated;
		this->bReason = NMConnectionStateReason::None;
		this->mActiveConnection = nullptr;
	}
};

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
}

void NMWirelessDevice::initWireless() {
	// clang-format off
	QObject::connect(this->wirelessProxy, &DBusNMWirelessProxy::AccessPointAdded, this, &NMWirelessDevice::onAccessPointPathAdded);
	QObject::connect(this->wirelessProxy, &DBusNMWirelessProxy::AccessPointRemoved, this, &NMWirelessDevice::onAccessPointPathRemoved);
	QObject::connect(this, &NMWirelessDevice::accessPointLoaded, this, &NMWirelessDevice::onAccessPointLoaded);
	QObject::connect(this, &NMWirelessDevice::connectionLoaded, this, &NMWirelessDevice::onConnectionLoaded);
	QObject::connect(this, &NMWirelessDevice::activeConnectionLoaded, this, &NMWirelessDevice::onActiveConnectionLoaded);
	// clang-format on
	this->registerAccessPoints();
}

void NMWirelessDevice::onAccessPointPathAdded(const QDBusObjectPath& path) {
	this->registerAccessPoint(path.path());
}

void NMWirelessDevice::onAccessPointPathRemoved(const QDBusObjectPath& path) {
	auto* ap = this->mAccessPoints.take(path.path());
	if (!ap) {
		qCDebug(logNetworkManager) << "Sent removal signal for" << path.path()
		                           << "which is not registered.";
		return;
	}
	emit ap->disappeared();
	delete ap;
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

	this->mAccessPoints.insert(path, ap);
	QObject::connect(
	    ap,
	    &NMAccessPoint::ready,
	    this,
	    [this, ap]() { emit this->accessPointLoaded(ap); },
	    Qt::SingleShotConnection
	);
}

// Only make WifiNetworks visible to the frontend WifiDevice when
// scanning is enabled or the network is connected or the network has known settings.
void NMWirelessDevice::updateNetworkVisibility(WifiNetwork* net) {
	const bool show = this->mScanning || net->connected() || net->known();
	const bool visible = this->mVisibleNetworks.contains(net);

	if (show && !visible) {
		this->mVisibleNetworks.insert(net);
		emit this->networkAdded(net);
	} else if (!show && visible) {
		this->mVisibleNetworks.remove(net);
		emit this->networkRemoved(net);
	}
}

NMWirelessNetwork* NMWirelessDevice::registerNetwork(const QString& ssid) {
	auto* backend = new NMWirelessNetwork(ssid, this);
	backend->bindableActiveApPath().setBinding([this]() { return this->activeApPath().path(); });
	backend->bindableCapabilities().setBinding([this]() { return this->capabilities(); });

	auto* frontend = new WifiNetwork(ssid, this);
	frontend->bindableSignalStrength().setBinding([backend]() {
		return backend->signalStrength() / 100.0;
	});
	frontend->bindableConnected().setBinding([backend]() {
		return backend->state() == NMConnectionState::Activated;
	});
	frontend->bindableKnown().setBinding([backend]() { return backend->known(); });
	frontend->bindableNmReason().setBinding([backend]() { return backend->reason(); });
	frontend->bindableSecurity().setBinding([backend]() { return backend->security(); });
	QObject::connect(backend, &NMWirelessNetwork::disappeared, this, [this, frontend, backend]() {
		QObject::disconnect(backend, nullptr, nullptr, nullptr);
		QObject::disconnect(frontend, nullptr, nullptr, nullptr);
		this->mBackendNetworks.remove(backend->ssid());
		this->mNetworks.remove(frontend->name());
		if (this->mVisibleNetworks.contains(frontend)) {
			this->mVisibleNetworks.remove(frontend);
			emit this->networkRemoved(frontend);
		}
		delete frontend;
		delete backend;
	});
	QObject::connect(frontend, &WifiNetwork::requestConnect, this, [this, backend]() {
		if (backend->referenceConnection()) {
			emit this->activateConnection(
			    QDBusObjectPath(backend->referenceConnection()->path()),
			    QDBusObjectPath(this->path())
			);
			return;
		}
		if (backend->referenceAp()) {
			emit this->addAndActivateConnection(
			    ConnectionSettingsMap(),
			    QDBusObjectPath(this->path()),
			    QDBusObjectPath(backend->referenceAp()->path())
			);
		}
	});
	QObject::connect(frontend, &WifiNetwork::connectedChanged, this, [this, frontend]() {
		this->updateNetworkVisibility(frontend);
	});
	QObject::connect(frontend, &WifiNetwork::knownChanged, this, [this, frontend]() {
		this->updateNetworkVisibility(frontend);
	});

	this->mBackendNetworks.insert(ssid, backend);
	this->mNetworks.insert(ssid, frontend);
	this->updateNetworkVisibility(frontend);
	return backend;
}

void NMWirelessDevice::onAccessPointLoaded(NMAccessPoint* ap) {
	const QString ssid = ap->ssid();
	if (!ssid.isEmpty()) {
		auto* net = this->mBackendNetworks.value(ssid);
		if (!net) net = this->registerNetwork(ssid);
		net->addAccessPoint(ap);
	}
}

void NMWirelessDevice::onConnectionLoaded(NMConnectionSettings* conn) {
	const ConnectionSettingsMap& settings = conn->settings();
	// Skip connections that have empty settings,
	// or who are this devices own hotspots.
	if (settings["connection"]["id"].toString().isEmpty()
	    || settings["connection"]["uuid"].toString().isEmpty()
	    || !settings.contains("802-11-wireless")
	    || settings["802-11-wireless"]["mode"].toString() == "ap"
	    || settings["802-11-wireless"]["ssid"].toString().isEmpty())
	{
		return;
	}

	const auto ssid = settings["802-11-wireless"]["ssid"].toString();
	auto* net = this->mBackendNetworks.value(ssid);
	if (!net) net = this->registerNetwork(ssid);

	net->addConnectionSettings(conn);
	auto* active = this->activeConnection();
	if (active && conn->path() == active->connection().path()) {
		net->addActiveConnection(active);
	}
}

void NMWirelessDevice::onActiveConnectionLoaded(NMActiveConnection* active) {
	const QString activeConnPath = active->connection().path();
	for (auto* net: this->mBackendNetworks.values()) {
		for (auto* conn: net->connections()) {
			if (activeConnPath == conn->path()) {
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
		// Rate limit if backend scan property updated within the interval
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

void NMWirelessDevice::handleScanner(bool enabled) {
	if (this->mScanning == enabled) return;
	this->mScanning = enabled;
	for (WifiNetwork* net: this->mNetworks) {
		this->updateNetworkVisibility(net);
	}
	enabled ? this->onScanTimeout() : this->mScanTimer.stop();
}

bool NMWirelessDevice::isWirelessValid() const {
	return this->wirelessProxy && this->wirelessProxy->isValid();
}

} // namespace qs::network

namespace qs::dbus {

DBusResult<qs::network::NMWirelessCapabilities::Enum>
DBusDataTransform<qs::network::NMWirelessCapabilities::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<qs::network::NMWirelessCapabilities::Enum>(wire));
}

DBusResult<QDateTime> DBusDataTransform<QDateTime>::fromWire(qint64 wire) {
	return DBusResult(network::clockBootTimeToDateTime(wire));
}

} // namespace qs::dbus
