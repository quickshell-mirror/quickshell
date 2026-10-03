#include "network.hpp"
#include <utility>

#include <qdbusconnection.h>
#include <qdbusextratypes.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qpointer.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/logcat.hpp"
#include "../enums.hpp"
#include "../known_network.hpp"
#include "../network.hpp"
#include "../wifi.hpp"
#include "accesspoint.hpp"
#include "active_connection.hpp"
#include "connection.hpp"
#include "enums.hpp"
#include "utils.hpp"

namespace qs::network {

namespace {
QS_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

NMNetwork::NMNetwork(QObject* parent)
    : QObject(parent)
    , bKnown(false)
    , bReason(NMConnectionStateReason::None)
    , bState(NMConnectionState::Deactivated) {}

void NMNetwork::updateReferenceConnection() {
	// If the network has no connections, the reference is nullptr.
	if (this->mConnections.isEmpty()) {
		this->bReferenceConnection = nullptr;
		return;
	};

	// If the network has an active connection, use its settings as the reference.
	if (auto* active = this->mActiveConnection) {
		if (auto* conn = this->mConnections.value(active->connection().path())) {
			if (conn != this->bReferenceConnection) this->bReferenceConnection = conn;
			return;
		}
	}

	// Otherwise, choose the settings responsible for the last successful connection.
	NMConnection* selectedConn = nullptr;
	quint64 selectedTimestamp = 0;
	for (auto* conn: this->mConnections.values()) {
		const quint64 timestamp = conn->settings()["connection"]["timestamp"].toULongLong();
		if (!selectedConn || timestamp > selectedTimestamp) {
			selectedConn = conn;
			selectedTimestamp = timestamp;
		}
	}

	if (this->bReferenceConnection != selectedConn) {
		this->bReferenceConnection = selectedConn;
	}
}

void NMNetwork::addConnection(NMConnection* conn) {
	if (this->mConnections.contains(conn->path())) return;
	this->mConnections.insert(conn->path(), conn);
	this->bKnown = true;
	this->updateReferenceConnection();
	emit this->connectionAdded(conn);
};

void NMNetwork::removeConnection(NMConnection* conn) {
	if (this->mConnections.value(conn->path()) != conn) return;
	this->mConnections.remove(conn->path());
	this->updateReferenceConnection();
	if (this->mConnections.isEmpty()) this->bKnown = false;
	emit this->connectionRemoved(conn);
}

void NMNetwork::addActiveConnection(NMActiveConnection* active) {
	if (this->mActiveConnection) return;
	this->mActiveConnection = active;
	this->updateReferenceConnection();
	this->bState.setBinding([active]() { return active->state(); });
	this->bReason.setBinding([active]() { return active->stateReason(); });
	auto onDestroyed = [this, active]() {
		if (this->mActiveConnection && this->mActiveConnection == active) {
			this->mActiveConnection = nullptr;
			this->updateReferenceConnection();
			this->bState = NMConnectionState::Deactivated;
			this->bReason = NMConnectionStateReason::None;
		}
	};
	QObject::connect(active, &NMActiveConnection::destroyed, this, onDestroyed);
}

void NMNetwork::forget() {
	if (this->mConnections.isEmpty()) return;
	for (auto* conn: this->mConnections.values()) {
		conn->forget();
	}
}

void NMNetwork::bindFrontend(Network* frontend) {
	auto translateState = [this]() { return this->state() == NMConnectionState::Activated; };
	frontend->bindableConnected().setBinding(translateState);
	frontend->bindableKnown().setBinding([this]() { return this->known(); });
	frontend->bindableState().setBinding([this]() {
		return static_cast<ConnectionState::Enum>(this->state());
	});
	frontend->bindableStateChanging().setBinding([this]() {
		auto s = static_cast<ConnectionState::Enum>(this->state());
		return s == ConnectionState::Connecting || s == ConnectionState::Disconnecting;
	});

	QObject::connect(this, &NMNetwork::reasonChanged, this, [this, frontend]() {
		if (this->reason() == NMConnectionStateReason::DeviceDisconnected) {
			auto deviceReason = this->deviceFailReason();
			if (deviceReason == NMDeviceStateReason::NoSecrets)
				emit frontend->connectionFailed(ConnectionFailReason::NoSecrets);
			if (deviceReason == NMDeviceStateReason::SupplicantDisconnect)
				emit frontend->connectionFailed(ConnectionFailReason::WifiClientDisconnected);
			if (deviceReason == NMDeviceStateReason::SupplicantFailed)
				emit frontend->connectionFailed(ConnectionFailReason::WifiClientFailed);
			if (deviceReason == NMDeviceStateReason::SupplicantTimeout)
				emit frontend->connectionFailed(ConnectionFailReason::WifiAuthTimeout);
			if (deviceReason == NMDeviceStateReason::SsidNotFound)
				emit frontend->connectionFailed(ConnectionFailReason::WifiNetworkLost);
		}
	});

	QObject::connect(
	    frontend,
	    &Network::requestConnectWithKnownNetwork,
	    this,
	    [this](KnownNetwork* knownNet) {
		    for (auto* conn: this->mConnections) {
			    if (conn->frontend() == knownNet) {
				    emit this->requestActivateConnection(conn->path());
				    return;
			    }
		    }
		    qCInfo(
		        logNetworkManager
		    ) << "Failed to connectToKnownNetwork: The provided profile no longer exists.";
	    }
	);

	QObject::connect(this, &NMNetwork::connectionAdded, frontend, [frontend](NMConnection* conn) {
		frontend->knownNetworkAdded(conn->frontend());
	});
	QObject::connect(this, &NMNetwork::connectionRemoved, frontend, [frontend](NMConnection* conn) {
		frontend->knownNetworkRemoved(conn->frontend());
	});

	// clang-format off
	QObject::connect(frontend, &Network::requestForget, this, &NMNetwork::forget);
	QObject::connect(frontend, &Network::requestDisconnect, this, &NMNetwork::requestDisconnect);
	// clang-format on
}

NMGenericNetwork::NMGenericNetwork(QString name, NetworkDevice* device, QObject* parent)
    : NMNetwork(parent)
    , mFrontend(new Network(std::move(name), device, this)) {
	// Regiter and bind the frontend Network.
	this->bindFrontend();
}

void NMGenericNetwork::bindFrontend() {
	auto* frontend = this->mFrontend;
	this->NMNetwork::bindFrontend(frontend);
	QObject::connect(frontend, &Network::requestConnect, this, [this]() {
		if (auto* settingsRef = this->referenceConnection()) {
			emit this->requestActivateConnection(settingsRef->path());
			return;
		}
		emit this->requestAddAndActivateConnection(NMSettings(), "/");
		return;
	});
}

NMWirelessNetwork::NMWirelessNetwork(const QString& ssid, NetworkDevice* device, QObject* parent)
    : NMNetwork(parent)
    , mSsid(ssid)
    , bSecurity(WifiSecurityType::Unknown) {

	auto updateSecurity = [this]() {
		if (NMConnection* conn = this->bReferenceConnection) {
			this->bSecurity.setBinding([conn]() { return securityFromSettings(conn->settings()); });
		} else if (NMAccessPoint* ap = this->bReferenceAp) {
			this->bSecurity.setBinding([ap]() { return ap->security(); });
		} else {
			this->bSecurity = WifiSecurityType::Unknown;
		}
		return;
	};

	auto checkDisappeared = [this]() {
		if (this->mAccessPoints.isEmpty() && this->mConnections.isEmpty()) emit this->disappeared();
	};

	QObject::connect(this, &NMWirelessNetwork::referenceConnectionChanged, this, updateSecurity);
	QObject::connect(this, &NMWirelessNetwork::referenceApChanged, this, updateSecurity);
	QObject::connect(this, &NMWirelessNetwork::connectionRemoved, this, checkDisappeared);
	QObject::connect(this, &NMWirelessNetwork::apRemoved, this, checkDisappeared);

	// Register and bind the frontend WifiNetwork.
	this->mFrontend = new WifiNetwork(ssid, device, this);
	this->bindFrontend();
}

void NMWirelessNetwork::updateReferenceAp() {
	// If the network has no APs, the reference is a nullptr.
	if (this->mAccessPoints.isEmpty()) {
		this->bReferenceAp = nullptr;
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
	if (this->bReferenceAp != selectedAp) {
		this->bReferenceAp = selectedAp;
		this->bSignalStrength.setBinding([selectedAp]() { return selectedAp->signalStrength(); });
	}
}

void NMWirelessNetwork::addAccessPoint(NMAccessPoint* ap) {
	const QString path = ap->path();
	if (this->mAccessPoints.contains(path)) return;
	this->mAccessPoints.insert(path, ap);
	auto onDestroyed = [this, ap, path]() {
		if (this->mAccessPoints.take(path)) {
			this->updateReferenceAp();
			// Deletes `this`
			emit this->apRemoved(ap);
		}
	};
	// clang-format off
	QObject::connect(ap, &NMAccessPoint::signalStrengthChanged, this, &NMWirelessNetwork::updateReferenceAp);
	QObject::connect(ap, &NMAccessPoint::destroyed, this, onDestroyed);
	// clang-format on
	this->updateReferenceAp();
};

void NMWirelessNetwork::bindFrontend() {
	auto* frontend = this->mFrontend;
	this->NMNetwork::bindFrontend(frontend);

	auto translateSignal = [this]() { return this->signalStrength() / 100.0; };
	frontend->bindableSignalStrength().setBinding(translateSignal);
	frontend->bindableSecurity().setBinding([this]() { return this->security(); });

	QObject::connect(frontend, &WifiNetwork::requestConnect, this, [this]() {
		if (auto* settingsRef = this->referenceConnection()) {
			emit this->requestActivateConnection(settingsRef->path());
			return;
		}
		if (auto* apRef = this->referenceAp()) {
			emit this->requestAddAndActivateConnection(NMSettings(), apRef->path());
			return;
		}
		emit this->requestAddAndActivateConnection(NMSettings(), "/");
		return;
	});

	QObject::connect(frontend, &WifiNetwork::requestConnectWithPsk, this, [this](const QString& psk) {
		NMSettings settings;
		settings["802-11-wireless-security"]["psk"] = psk;
		if (const QPointer<NMConnection> ref = this->referenceConnection()) {
			auto* call = ref->update(settings);
			QObject::connect(
			    call,
			    &QDBusPendingCallWatcher::finished,
			    this,
			    [this, ref](QDBusPendingCallWatcher* call) {
				    const QDBusPendingReply<> reply = *call;
				    if (reply.isError()) {
					    qCInfo(logNetworkManager) << "Failed to write PSK: " << reply.error().message();
				    } else {
					    if (!ref) {
						    qCInfo(logNetworkManager) << "Failed to connectWithPsk: The settings disappeared.";
					    } else {
						    emit this->requestActivateConnection(ref->path());
					    }
				    }
				    delete call;
			    }
			);
			return;
		}
		if (auto* apRef = this->referenceAp()) {
			emit this->requestAddAndActivateConnection(settings, apRef->path());
			return;
		}
		qCInfo(logNetworkManager) << "Failed to connectWithPsk: The network disappeared.";
	});
}

} // namespace qs::network
