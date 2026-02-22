#include "wireless.hpp"
#include <utility>

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
#include "../nm_settings.hpp"
#include "../wifi.hpp"
#include "accesspoint.hpp"
#include "connection.hpp"
#include "dbus_nm_wireless.h"
#include "device.hpp"
#include "enums.hpp"
#include "types.hpp"
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
    , bSecurity(WifiSecurityType::Unknown)
    , bReason(NMNetworkStateReason::None)
    , bState(NMConnectionState::Deactivated) {}

void NMWirelessNetwork::updateReferenceConnection() {
	// If the network has no connections, the reference is nullptr.
	if (this->mConnections.isEmpty()) {
		this->mReferenceConn = nullptr;
		this->bSecurity = WifiSecurityType::Unknown;
		// Set security back to reference AP.
		if (this->mReferenceAp) {
			this->bSecurity.setBinding([this]() { return this->mReferenceAp->security(); });
		}
		return;
	};

	// If the network has an active connection, use it as the reference.
	if (this->mActiveConnection) {
		auto* conn = this->mConnections.value(this->mActiveConnection->connection().path());
		if (conn && conn != this->mReferenceConn) {
			this->mReferenceConn = conn;
			this->bSecurity.setBinding([conn]() { return conn->security(); });
		}
		return;
	}

	// Otherwise, choose the connection with the strongest security settings.
	NMConnectionSettings* selectedConn = nullptr;
	for (auto* conn: this->mConnections.values()) {
		if (!selectedConn || conn->security() > selectedConn->security()) {
			selectedConn = conn;
		}
	}
	if (this->mReferenceConn != selectedConn) {
		this->mReferenceConn = selectedConn;
		this->bSecurity.setBinding([selectedConn]() { return selectedConn->security(); });
	}
}

void NMWirelessNetwork::updateReferenceAp() {
	// If the network has no APs, the reference is a nullptr.
	if (this->mAccessPoints.isEmpty()) {
		this->mReferenceAp = nullptr;
		this->bSignalStrength = 0;
		return;
	}

	// Otherwise, choose the AP with the strongest signal.
	NMAccessPoint* selectedAp = nullptr;
	for (auto* ap: this->mAccessPoints.values()) {
		// Always prefer the active AP.
		if (ap->path() == this->bActiveApPath) {
			selectedAp = ap;
			break;
		}
		if (!selectedAp || ap->signalStrength() > selectedAp->signalStrength()) {
			selectedAp = ap;
		}
	}
	if (this->mReferenceAp != selectedAp) {
		this->mReferenceAp = selectedAp;
		this->bSignalStrength.setBinding([selectedAp]() { return selectedAp->signalStrength(); });
		// Reference AP is used for security when there's no connection settings.
		if (!this->mReferenceConn) {
			this->bSecurity.setBinding([selectedAp]() { return selectedAp->security(); });
		}
	}
}

void NMWirelessNetwork::addAccessPoint(NMAccessPoint* ap) {
	if (this->mAccessPoints.contains(ap->path())) return;
	this->mAccessPoints.insert(ap->path(), ap);
	auto onDestroyed = [this, ap]() {
		if (this->mAccessPoints.take(ap->path())) {
			this->updateReferenceAp();
			if (this->mAccessPoints.isEmpty() && this->mConnections.isEmpty()) emit this->disappeared();
		}
	};
	// clang-format off
	QObject::connect(ap, &NMAccessPoint::signalStrengthChanged, this, &NMWirelessNetwork::updateReferenceAp);
	QObject::connect(ap, &NMAccessPoint::destroyed, this, onDestroyed);
	// clang-format on
	this->updateReferenceAp();
};

void NMWirelessNetwork::addConnection(NMConnectionSettings* conn) {
	if (this->mConnections.contains(conn->path())) return;
	this->mConnections.insert(conn->path(), conn);
	this->registerFrontendSettings(conn);

	auto onDestroyed = [this, conn]() {
		if (this->mConnections.take(conn->path())) {
			this->removeFrontendSettings(conn);
			this->updateReferenceConnection();
			if (this->mConnections.isEmpty()) this->bKnown = false;
			if (this->mAccessPoints.isEmpty() && this->mConnections.isEmpty()) emit this->disappeared();
		}
	};
	// clang-format off
	QObject::connect(conn, &NMConnectionSettings::securityChanged, this, &NMWirelessNetwork::updateReferenceConnection);
	QObject::connect(conn, &NMConnectionSettings::destroyed, this, onDestroyed);
	// clang-format on
	this->bKnown = true;
	this->updateReferenceConnection();
};

void NMWirelessNetwork::registerFrontendSettings(NMConnectionSettings* conn) {
	auto* frontendConn = new NMWifiSettings(conn);

	// clang-format off
	frontendConn->bindableId().setBinding([conn]() { return conn->id(); });
	frontendConn->bindableWifiSecurity().setBinding([conn]() { return conn->security(); });
	QObject::connect(frontendConn, &NMWifiSettings::requestClearSecrets, conn, &NMConnectionSettings::clearSecrets);
	QObject::connect(frontendConn, &NMWifiSettings::requestForget, conn, &NMConnectionSettings::forget);
	QObject::connect(frontendConn, &NMWifiSettings::requestSetWifiPsk, conn, &NMConnectionSettings::setWifiPsk);
	// clang-format on

	this->mFrontendSettings.insert(conn->path(), frontendConn);
	emit this->settingsAdded(frontendConn);
}

void NMWirelessNetwork::removeFrontendSettings(NMConnectionSettings* conn) {
	auto* frontendConn = this->mFrontendSettings.take(conn->path());
	if (frontendConn) {
		emit this->settingsRemoved(frontendConn);
		frontendConn->deleteLater();
	}
}

void NMWirelessNetwork::addActiveConnection(NMActiveConnection* active) {
	if (this->mActiveConnection) return;
	this->mActiveConnection = active;

	auto* settings = this->mFrontendSettings.value(this->mActiveConnection->connection().path());
	if (settings) {
		this->bActiveSettings = settings;
		QObject::connect(settings, &NMSettings::destroyed, this, [this]() {
			this->bActiveSettings = nullptr;
		});
	}

	this->bState.setBinding([active]() { return active->state(); });
	this->bReason.setBinding([active]() { return active->stateReason(); });
	auto onDestroyed = [this, active]() {
		if (this->mActiveConnection && this->mActiveConnection == active) {
			this->mActiveConnection = nullptr;
			this->bActiveSettings = nullptr;
			this->updateReferenceConnection();
			this->bState = NMConnectionState::Deactivated;
			this->bReason = NMNetworkStateReason::None;
		}
	};
	QObject::connect(active, &NMActiveConnection::destroyed, this, onDestroyed);
	this->updateReferenceConnection();
};

NMConnectionSettings* NMWirelessNetwork::getConnectionFromSettings(NMSettings* settings) {
	for (auto it = this->mFrontendSettings.begin(); it != this->mFrontendSettings.end(); ++it) {
		if (it.value() == settings) {
			return this->mConnections.value(it.key());
		}
	}
	return nullptr;
}

void NMWirelessNetwork::forget() {
	if (this->mConnections.isEmpty()) return;
	for (auto* conn: this->mConnections.values()) {
		conn->forget();
	}
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
}

void NMWirelessDevice::initWireless() {
	// clang-format off
	QObject::connect(this->wirelessProxy, &DBusNMWirelessProxy::AccessPointAdded, this, &NMWirelessDevice::onAccessPointAdded);
	QObject::connect(this->wirelessProxy, &DBusNMWirelessProxy::AccessPointRemoved, this, &NMWirelessDevice::onAccessPointRemoved);
	QObject::connect(this, &NMWirelessDevice::accessPointLoaded, this, &NMWirelessDevice::onAccessPointLoaded);
	QObject::connect(this, &NMWirelessDevice::connectionLoaded, this, &NMWirelessDevice::onConnectionLoaded);
	QObject::connect(this, &NMWirelessDevice::activeConnectionLoaded, this, &NMWirelessDevice::onActiveConnectionLoaded);
	QObject::connect(this, &NMWirelessDevice::scanningChanged, this, &NMWirelessDevice::onScanningChanged);
	// clang-format on
	this->registerAccessPoints();
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

void NMWirelessDevice::onConnectionLoaded(NMConnectionSettings* conn) {
	const ConnectionSettingsMap& settings = conn->settings();
	// Filter connections that aren't wireless or have missing settings
	if (settings["connection"]["id"].toString().isEmpty()
	    || settings["connection"]["uuid"].toString().isEmpty()
	    || !settings.contains("802-11-wireless")
	    || settings["802-11-wireless"]["ssid"].toString().isEmpty())
	{
		return;
	}

	const auto ssid = settings["802-11-wireless"]["ssid"].toString();
	const auto mode = settings["802-11-wireless"]["mode"].toString();

	if (mode == "infrastructure") {
		auto* net = this->mNetworks.value(ssid);
		if (!net) net = this->registerNetwork(ssid);
		net->addConnection(conn);

		// Check for active connections that loaded before their respective connection settings
		auto* active = this->activeConnection();
		if (active && conn->path() == active->connection().path()) {
			net->addActiveConnection(active);
		}
	}
	// TODO: Create hotspots when mode == "ap"
}

void NMWirelessDevice::onActiveConnectionLoaded(NMActiveConnection* active) {
	// Find an exisiting network with connection settings that matches the active
	const QString activeConnPath = active->connection().path();
	for (const auto& net: this->mNetworks.values()) {
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
	auto* net = new NMWirelessNetwork(ssid, this);

	// To avoid exposing outdated state to the frontend, filter the backend networks to only show
	// the known or currently connected networks when the scanner is off.
	auto visible = [this, net]() {
		return this->bScanning || net->state() == NMConnectionState::Activated || net->known();
	};
	auto onVisibilityChanged = [this, net](bool visible) {
		visible ? this->registerFrontendNetwork(net) : this->removeFrontendNetwork(net);
	};

	net->bindableVisible().setBinding(visible);
	net->bindableActiveApPath().setBinding([this]() { return this->activeApPath().path(); });
	net->bindableDeviceFailReason().setBinding([this]() { return this->lastFailReason(); });
	QObject::connect(net, &NMWirelessNetwork::disappeared, this, &NMWirelessDevice::removeNetwork);
	QObject::connect(net, &NMWirelessNetwork::visibilityChanged, this, onVisibilityChanged);

	this->mNetworks.insert(ssid, net);
	if (net->visible()) this->registerFrontendNetwork(net);
	return net;
}

void NMWirelessDevice::registerFrontendNetwork(NMWirelessNetwork* net) {
	auto ssid = net->ssid();
	auto* frontendNet = new WifiNetwork(ssid, net);

	// Bind WifiNetwork to NMWirelessNetwork
	auto translateSignal = [net]() { return net->signalStrength() / 100.0; };
	auto translateState = [net]() { return net->state() == NMConnectionState::Activated; };
	frontendNet->bindableSignalStrength().setBinding(translateSignal);
	frontendNet->bindableConnected().setBinding(translateState);
	frontendNet->bindableKnown().setBinding([net]() { return net->known(); });
	frontendNet->bindableNmStateReason().setBinding([net]() { return net->reason(); });
	frontendNet->bindableNmDeviceFailReason().setBinding([net]() { return net->deviceFailReason(); });
	frontendNet->bindableActiveSettings().setBinding([net]() { return net->activeSettings(); });
	frontendNet->bindableSecurity().setBinding([net]() { return net->security(); });
	frontendNet->bindableState().setBinding([net]() {
		return static_cast<NetworkState::Enum>(net->state());
	});

	QObject::connect(frontendNet, &WifiNetwork::requestConnect, this, [this, net]() {
		if (net->referenceConnection()) {
			emit this->activateConnection(
			    QDBusObjectPath(net->referenceConnection()->path()),
			    QDBusObjectPath(this->path())
			);
			return;
		}
		if (net->referenceAp()) {
			emit this->addAndActivateConnection(
			    ConnectionSettingsMap(),
			    QDBusObjectPath(this->path()),
			    QDBusObjectPath(net->referenceAp()->path())
			);
		}
	});

	QObject::connect(
	    frontendNet,
	    &WifiNetwork::requestConnectWithSettings,
	    this,
	    [this, net](NMSettings* settings) {
		    auto* conn = net->getConnectionFromSettings(settings);
		    if (conn) {
			    emit this->activateConnection(
			        QDBusObjectPath(conn->path()),
			        QDBusObjectPath(this->path())
			    );
		    }
	    }
	);

	// clang-format off
	QObject::connect(frontendNet, &WifiNetwork::requestDisconnect, this, &NMWirelessDevice::disconnect);
	QObject::connect(frontendNet, &WifiNetwork::requestForget, net, &NMWirelessNetwork::forget);
	QObject::connect(net, &NMWirelessNetwork::settingsAdded, frontendNet, &WifiNetwork::settingsAdded);
	QObject::connect(net, &NMWirelessNetwork::settingsRemoved, frontendNet, &WifiNetwork::settingsRemoved);
	// clang-format on

	for (NMSettings* frontendConn: net->frontendSettings()) {
		emit frontendNet->settingsAdded(frontendConn);
	};

	this->mFrontendNetworks.insert(ssid, frontendNet);
	emit this->networkAdded(frontendNet);
}

void NMWirelessDevice::removeFrontendNetwork(NMWirelessNetwork* net) {
	auto* frontendNet = this->mFrontendNetworks.take(net->ssid());
	if (frontendNet) {
		emit this->networkRemoved(frontendNet);
		frontendNet->deleteLater();
	}
}

void NMWirelessDevice::removeNetwork() {
	auto* net = qobject_cast<NMWirelessNetwork*>(this->sender());
	if (this->mNetworks.take(net->ssid())) {
		this->removeFrontendNetwork(net);
		delete net;
	};
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
