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

NMWirelessNetwork::NMWirelessNetwork(QString ssid, QObject* parent)
    : QObject(parent)
    , mSsid(std::move(ssid)) {}

void NMWirelessNetwork::updateReferenceConnection() {
	// If the network has no connections, the reference is nullptr.
	if (this->mConnections.isEmpty()) {
		this->mReferenceConn = nullptr;
	};

	// If the network has an active connection, use it as the reference.
	if (this->mActiveConnection) {
		auto* ref = mConnections.value(this->mActiveConnection->connection().path());
		if (ref) {
			this->mReferenceConn = ref;
			return;
		}
	}

	// Otherwise, choose the connection with the strongest security settings.
	NMWirelessSecurityType::Enum selectedSecurity = NMWirelessSecurityType::Unknown;
	NMConnectionSettings* selectedConn = nullptr;
	for (auto* conn: this->mConnections) {
		NMWirelessSecurityType::Enum security = conn->security();
		if (selectedSecurity >= security) {
			selectedSecurity = security;
			selectedConn = conn;
		}
	}

	if (selectedConn && this->mReferenceConn != selectedConn) {
		this->mReferenceConn = selectedConn;
	}
}

void NMWirelessNetwork::setState(NMConnectionState::Enum state) {
	if (this->mState == state) return;
	this->mState = state;
	emit this->stateChanged(state);
}

void NMWirelessNetwork::setReason(NMConnectionStateReason::Enum reason) {
	if (this->mReason == reason) return;
	this->mReason = reason;
	emit this->reasonChanged(reason);
};

void NMWirelessNetwork::setKnown(bool known) {
	if (this->mKnown == known) return;
	this->mKnown = known;
	emit this->knownChanged(known);
}

void NMWirelessNetwork::updateSignalStrength() {
	quint8 selectedStrength = 0;
	NMAccessPoint* selectedAp = nullptr;

	for (auto* ap: this->mAccessPoints.values()) {
		if (selectedStrength <= ap->signalStrength()) {
			selectedStrength = ap->signalStrength();
			selectedAp = ap;
		}
	}

	if (selectedStrength != this->mSignalStrength) {
		this->mSignalStrength = selectedStrength;
		emit this->signalStrengthChanged(selectedStrength);
	}

	if (selectedAp && this->mReferenceAp != selectedAp) {
		this->mReferenceAp = selectedAp;
		NMWirelessSecurityType::Enum selectedSecurity = selectedAp->security();
		if (this->mSecurity != selectedSecurity) {
			this->mSecurity = selectedSecurity;
			emit this->securityChanged(selectedSecurity);
		}
	}
}

void NMWirelessNetwork::addAccessPoint(NMAccessPoint* ap) {
	if (this->mAccessPoints.contains(ap->path())) return;
	this->mAccessPoints.insert(ap->path(), ap);
	// clang-format off
	QObject::connect(ap, &NMAccessPoint::signalStrengthChanged, this, &NMWirelessNetwork::updateSignalStrength);
	QObject::connect(ap, &NMAccessPoint::disappeared, this, [this, ap] { this->removeAccessPoint(ap); });
	// clang-format on
	this->updateSignalStrength();
};

void NMWirelessNetwork::removeAccessPoint(NMAccessPoint* ap) {
	auto* found = this->mAccessPoints.take(ap->path());
	if (!found) {
		qCWarning(logNetworkManager) << "Backend network" << this->ssid() << "is not in sync!";
	} else {
		if (this->mAccessPoints.isEmpty()) {
			emit this->disappeared();
		} else {
			QObject::disconnect(ap, nullptr, this, nullptr);
			this->updateSignalStrength();
		}
	}
};

void NMWirelessNetwork::addConnectionSettings(NMConnectionSettings* conn) {
	if (this->mConnections.contains(conn->path())) return;
	this->mConnections.insert(conn->path(), conn);
	// clang-format off
	QObject::connect(conn, &NMConnectionSettings::securityChanged, this, &NMWirelessNetwork::updateReferenceConnection);
	QObject::connect(conn, &NMConnectionSettings::disappeared, this, [this, conn]() { this->removeConnectionSettings(conn); });
	// clang-format on
	this->updateReferenceConnection();
	this->setKnown(true);
};

void NMWirelessNetwork::removeConnectionSettings(NMConnectionSettings* conn) {
	auto* found = this->mConnections.take(conn->path());
	if (!found) {
		qCWarning(logNetworkManager) << "Backend network" << this->ssid() << "is not in sync!";
	} else {
		QObject::disconnect(conn, nullptr, this, nullptr);
		this->updateReferenceConnection();
		if (mConnections.isEmpty()) {
			this->setKnown(false);
		}
	}
};

void NMWirelessNetwork::addActiveConnection(NMActiveConnection* active) {
	if (this->mActiveConnection) {
		qCWarning(logNetworkManager) << "Backend network" << this->ssid() << "is not in sync!";
		return;
	}
	this->setState(active->state());
	this->setReason(active->stateReason());
	this->mActiveConnection = active;
	// clang-format off
	QObject::connect(active, &NMActiveConnection::stateChanged, this, &NMWirelessNetwork::setState);
	QObject::connect(active, &NMActiveConnection::stateReasonChanged, this, &NMWirelessNetwork::setReason);
	QObject::connect(active, &NMActiveConnection::disappeared, this, [this, active] { this->removeActiveConnection(active); });
	// clang-format on
};

void NMWirelessNetwork::removeActiveConnection(NMActiveConnection* active) {
	if (this->mActiveConnection && this->mActiveConnection == active) {
		QObject::disconnect(active, nullptr, this, nullptr);
		this->setState(NMConnectionState::Deactivated);
		this->setReason(NMConnectionStateReason::None);
		this->mActiveConnection = nullptr;
	} else {
		qCWarning(logNetworkManager) << "Backend network" << this->ssid() << "is not in sync!";
	}
};

NMWirelessDevice::NMWirelessDevice(const QString& path, QObject* parent): NMDevice(path, parent) {
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
	auto* ap = mAccessPoints.take(path.path());

	if (!ap) {
		qCDebug(logNetworkManager) << "NetworkManager sent removal signal for" << path.path()
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

	auto* ap = new NMAccessPoint(path, this->capabilities(), this);

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

NMWirelessNetwork* NMWirelessDevice::registerNetwork(const QString& ssid) {
	auto* frontend = new WifiNetwork(ssid, this);
	auto* backend = new NMWirelessNetwork(ssid, this);

	// clang-format off
	frontend->setSignalStrength(backend->signalStrength());
	frontend->setState(static_cast<NetworkConnectionState::Enum>(backend->state()));
	frontend->setKnown(backend->known());
	frontend->setNmReason(backend->reason());
	frontend->setNmSecurity(backend->security());
	QObject::connect(backend, &NMWirelessNetwork::signalStrengthChanged, frontend, &WifiNetwork::setSignalStrength); 
	QObject::connect(backend, &NMWirelessNetwork::knownChanged, frontend, &WifiNetwork::setKnown); 
	QObject::connect(backend, &NMWirelessNetwork::reasonChanged, frontend, &WifiNetwork::setNmReason); 
	QObject::connect(backend, &NMWirelessNetwork::securityChanged, frontend, &WifiNetwork::setNmSecurity);
	QObject::connect(backend, &NMWirelessNetwork::stateChanged, frontend, 
			[frontend](NMConnectionState::Enum state) { frontend->setState(static_cast<NetworkConnectionState::Enum>(state)); }
	);
	// clang-format on
	QObject::connect(backend, &NMWirelessNetwork::disappeared, this, [this, frontend, backend]() {
		QObject::disconnect(backend, nullptr, nullptr, nullptr);
		emit this->wifiNetworkRemoved(frontend);
		this->mBackendNetworks.remove(backend->ssid());
		delete backend;
		delete frontend;
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

	this->mBackendNetworks.insert(ssid, backend);
	emit this->wifiNetworkAdded(frontend);
	return backend;
}

void NMWirelessDevice::onAccessPointLoaded(NMAccessPoint* ap) {
	QString ssid = ap->ssid();
	if (!ssid.isEmpty()) {
		auto* net = this->mBackendNetworks.value(ssid);
		if (!net) net = registerNetwork(ssid);
		net->addAccessPoint(ap);
	}
}

void NMWirelessDevice::onConnectionLoaded(NMConnectionSettings* conn) {
	const ConnectionSettingsMap& settings = conn->settings();
	// Early return for invalid connections
	if (settings["connection"]["id"].toString().isEmpty()
	    || settings["connection"]["uuid"].toString().isEmpty()
	    || !settings.contains("802-11-wireless")
	    || settings["802-11-wireless"]["mode"].toString() == "ap"
	    || settings["802-11-wireless"]["ssid"].toString().isEmpty())
	{
		return;
	}

	const QString ssid = settings["802-11-wireless"]["ssid"].toString();
	auto* net = mBackendNetworks.value(ssid);
	if (!net) net = registerNetwork(ssid);

	net->addConnectionSettings(conn);
	auto* active = this->activeConnection();
	if (active && conn->path() == active->connection().path()) {
		net->addActiveConnection(active);
	}
}

void NMWirelessDevice::onActiveConnectionLoaded(NMActiveConnection* active) {
	QString connPath = active->connection().path();
	for (auto* net: this->mBackendNetworks.values()) {
		for (auto* conn: net->connections()) {
			if (connPath == conn->path()) {
				net->addActiveConnection(active);
				return;
			}
		}
	}
}

void NMWirelessDevice::scan() { this->wirelessProxy->RequestScan({}); }
bool NMWirelessDevice::isWirelessValid() const {
	return this->wirelessProxy && this->wirelessProxy->isValid();
}

} // namespace qs::network
