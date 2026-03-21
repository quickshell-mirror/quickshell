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
#include "../wifi.hpp"
#include "accesspoint.hpp"
#include "active_connection.hpp"
#include "dbus_nm_wireless.h"
#include "dbus_types.hpp"
#include "device.hpp"
#include "enums.hpp"
#include "settings.hpp"
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
    , bReason(NMConnectionStateReason::None)
    , bState(NMConnectionState::Deactivated) {}

void NMWirelessNetwork::updateReferenceSettings() {
	// If the network has no connections, the reference is nullptr.
	if (this->mSettings.isEmpty()) {
		this->mReferenceSettings = nullptr;
		this->bSecurity = WifiSecurityType::Unknown;
		if (this->mReferenceAp) {
			this->bSecurity.setBinding([this]() { return this->mReferenceAp->security(); });
		}
		return;
	};

	// If the network has an active connection, use its settings as the reference.
	if (this->mActiveConnection) {
		auto* settings = this->mSettings.value(this->mActiveConnection->connection().path());
		if (settings && settings != this->mReferenceSettings) {
			this->mReferenceSettings = settings;
			this->bSecurity.setBinding([settings]() { return securityFromSettingsMap(settings->map()); });
		}
		return;
	}

	// Otherwise, choose the settings responsible for the last successful connection.
	NMSettings* selectedSettings = nullptr;
	quint64 selectedTimestamp = 0;
	for (auto* settings: this->mSettings.values()) {
		const quint64 timestamp = settings->map()["connection"]["timestamp"].toULongLong();
		if (!selectedSettings || timestamp > selectedTimestamp) {
			selectedSettings = settings;
			selectedTimestamp = timestamp;
		}
	}

	if (this->mReferenceSettings != selectedSettings) {
		this->mReferenceSettings = selectedSettings;
		this->bSecurity.setBinding([selectedSettings]() {
			return securityFromSettingsMap(selectedSettings->map());
		});
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
		if (!this->mReferenceSettings) {
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
			if (this->mAccessPoints.isEmpty() && this->mSettings.isEmpty()) emit this->disappeared();
		}
	};
	// clang-format off
	QObject::connect(ap, &NMAccessPoint::signalStrengthChanged, this, &NMWirelessNetwork::updateReferenceAp);
	QObject::connect(ap, &NMAccessPoint::destroyed, this, onDestroyed);
	// clang-format on
	this->updateReferenceAp();
};

void NMWirelessNetwork::addSettings(NMSettings* settings) {
	if (this->mSettings.contains(settings->path())) return;
	this->mSettings.insert(settings->path(), settings);

	auto onDestroyed = [this, settings]() {
		if (this->mSettings.take(settings->path())) {
			emit this->settingsRemoved(settings);
			this->updateReferenceSettings();
			if (this->mSettings.isEmpty()) this->bKnown = false;
			if (this->mAccessPoints.isEmpty() && this->mSettings.isEmpty()) emit this->disappeared();
		}
	};
	QObject::connect(settings, &NMSettings::destroyed, this, onDestroyed);
	this->bKnown = true;
	this->updateReferenceSettings();
	emit this->settingsAdded(settings);
};

void NMWirelessNetwork::addActiveConnection(NMActiveConnection* active) {
	if (this->mActiveConnection) return;
	this->mActiveConnection = active;

	this->bState.setBinding([active]() { return active->state(); });
	this->bReason.setBinding([active]() { return active->stateReason(); });
	auto onDestroyed = [this, active]() {
		if (this->mActiveConnection && this->mActiveConnection == active) {
			this->mActiveConnection = nullptr;
			this->updateReferenceSettings();
			this->bState = NMConnectionState::Deactivated;
			this->bReason = NMConnectionStateReason::None;
		}
	};
	QObject::connect(active, &NMActiveConnection::destroyed, this, onDestroyed);
	this->updateReferenceSettings();
};

void NMWirelessNetwork::forget() {
	if (this->mSettings.isEmpty()) return;
	for (auto* conn: this->mSettings.values()) {
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
	QObject::connect(this, &NMWirelessDevice::settingsLoaded, this, &NMWirelessDevice::onSettingsLoaded);
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
	// Find an exisiting network with connection settings that matches the active
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

		call->deleteLater();
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
	auto* net = new NMWirelessNetwork(ssid, this);

	auto visible = [this, net]() {
		return this->bScanning || net->state() == NMConnectionState::Activated || net->known();
	};

	net->bindableVisible().setBinding(visible);
	net->bindableActiveApPath().setBinding([this]() { return this->activeApPath().path(); });
	net->bindableDeviceFailReason().setBinding([this]() { return this->lastFailReason(); });
	QObject::connect(net, &NMWirelessNetwork::disappeared, this, &NMWirelessDevice::removeNetwork);

	qCDebug(logNetworkManager) << "Registered network for SSID" << ssid;
	this->mNetworks.insert(ssid, net);
	this->registerFrontendNetwork(net);
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
	frontendNet->bindableSecurity().setBinding([net]() { return net->security(); });
	frontendNet->bindableState().setBinding([net]() {
		return static_cast<ConnectionState::Enum>(net->state());
	});

	QObject::connect(net, &NMWirelessNetwork::reasonChanged, this, [net, frontendNet]() {
		if (net->reason() == NMConnectionStateReason::DeviceDisconnected) {
			auto deviceReason = net->deviceFailReason();
			if (deviceReason == NMDeviceStateReason::NoSecrets)
				emit frontendNet->connectionFailed(ConnectionFailReason::NoSecrets);
			if (deviceReason == NMDeviceStateReason::SupplicantDisconnect)
				emit frontendNet->connectionFailed(ConnectionFailReason::WifiClientDisconnected);
			if (deviceReason == NMDeviceStateReason::SupplicantFailed)
				emit frontendNet->connectionFailed(ConnectionFailReason::WifiClientFailed);
			if (deviceReason == NMDeviceStateReason::SupplicantTimeout)
				emit frontendNet->connectionFailed(ConnectionFailReason::WifiAuthTimeout);
			if (deviceReason == NMDeviceStateReason::SsidNotFound)
				emit frontendNet->connectionFailed(ConnectionFailReason::WifiNetworkLost);
		}
	});

	QObject::connect(frontendNet, &WifiNetwork::requestConnect, this, [this, net]() {
		if (net->referenceSettings()) {
			emit this->activateConnection(
			    QDBusObjectPath(net->referenceSettings()->path()),
			    QDBusObjectPath(this->path())
			);
			return;
		}
		if (net->referenceAp()) {
			emit this->addAndActivateConnection(
			    NMSettingsMap(),
			    QDBusObjectPath(this->path()),
			    QDBusObjectPath(net->referenceAp()->path())
			);
			return;
		}
		qCInfo(logNetworkManager) << "Failed to connect to"
		                          << this->path() + ": The network disappeared.";
	});

	QObject::connect(
	    frontendNet,
	    &WifiNetwork::requestConnectWithPsk,
	    this,
	    [this, net](const QString& psk) {
		    NMSettingsMap settings;
		    settings["802-11-wireless-security"]["psk"] = psk;
		    if (auto* ref = net->referenceSettings()) {
			    auto* call = ref->updateSettings(settings);
			    QObject::connect(
			        call,
			        &QDBusPendingCallWatcher::finished,
			        this,
			        [this, ref](QDBusPendingCallWatcher* call) {
				        const QDBusPendingReply<> reply = *call;

				        if (reply.isError()) {
					        qCInfo(logNetworkManager)
					            << "Failed to write PSK for" << this->path() + ":" << reply.error().message();
				        } else {
					        emit this->activateConnection(
					            QDBusObjectPath(ref->path()),
					            QDBusObjectPath(this->path())
					        );
				        }
				        delete call;
			        }
			    );
			    return;
		    }
		    if (net->referenceAp()) {
			    emit this->addAndActivateConnection(
			        settings,
			        QDBusObjectPath(this->path()),
			        QDBusObjectPath(net->referenceAp()->path())
			    );
			    return;
		    }
		    qCInfo(logNetworkManager) << "Failed to connectWithPsk to"
		                              << this->path() + ": The network disappeared.";
	    }
	);

	QObject::connect(
	    frontendNet,
	    &WifiNetwork::requestConnectWithSettings,
	    this,
	    [this](NMSettings* settings) {
		    if (settings) {
			    emit this->activateConnection(
			        QDBusObjectPath(settings->path()),
			        QDBusObjectPath(this->path())
			    );
			    return;
		    }
		    qCInfo(logNetworkManager) << "Failed to connectWithSettings to"
		                              << this->path() + ": The provided settings no longer exist.";
	    }
	);

	QObject::connect(
	    net,
	    &NMWirelessNetwork::visibilityChanged,
	    this,
	    [this, frontendNet](bool visible) {
		    if (visible) this->networkAdded(frontendNet);
		    else this->networkRemoved(frontendNet);
	    }
	);

	// clang-format off
	QObject::connect(frontendNet, &WifiNetwork::requestDisconnect, this, &NMWirelessDevice::disconnect);
	QObject::connect(frontendNet, &WifiNetwork::requestForget, net, &NMWirelessNetwork::forget);
	QObject::connect(net, &NMWirelessNetwork::settingsAdded, frontendNet, &WifiNetwork::settingsAdded);
	QObject::connect(net, &NMWirelessNetwork::settingsRemoved, frontendNet, &WifiNetwork::settingsRemoved);
	// clang-format on

	this->mFrontendNetworks.insert(ssid, frontendNet);
	if (net->visible()) emit this->networkAdded(frontendNet);
}

void NMWirelessDevice::removeFrontendNetwork(NMWirelessNetwork* net) {
	auto* frontendNet = this->mFrontendNetworks.take(net->ssid());
	if (frontendNet) {
		if (net->visible()) emit this->networkRemoved(frontendNet);
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
