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
#include "../network.hpp"
#include "../wifi.hpp"
#include "accesspoint.hpp"
#include "active_connection.hpp"
#include "enums.hpp"
#include "settings.hpp"
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

void NMNetwork::updateReferenceSettings() {
	// If the network has no connections, the reference is nullptr.
	if (this->mSettings.isEmpty()) {
		this->bReferenceSettings = nullptr;
		return;
	};

	// If the network has an active connection, use its settings as the reference.
	if (this->mActiveConnection) {
		auto* settings = this->mSettings.value(this->mActiveConnection->connection().path());
		if (settings && settings != this->bReferenceSettings) {
			this->bReferenceSettings = settings;
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

	if (this->bReferenceSettings != selectedSettings) {
		this->bReferenceSettings = selectedSettings;
	}
}

void NMNetwork::addSettings(NMSettings* settings) {
	if (this->mSettings.contains(settings->path())) return;
	this->mSettings.insert(settings->path(), settings);

	auto onDestroyed = [this, settings]() {
		if (this->mSettings.take(settings->path())) {
			emit this->settingsRemoved(settings);
			this->updateReferenceSettings();
			if (this->mSettings.isEmpty()) this->bKnown = false;
		}
	};
	QObject::connect(settings, &NMSettings::destroyed, this, onDestroyed);
	this->bKnown = true;
	this->updateReferenceSettings();
	emit this->settingsAdded(settings);
};

void NMNetwork::addActiveConnection(NMActiveConnection* active) {
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

void NMNetwork::forget() {
	if (this->mSettings.isEmpty()) return;
	for (auto* conn: this->mSettings.values()) {
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
	    &Network::requestConnectWithSettings,
	    this,
	    [this](NMSettings* settings) {
		    if (settings) {
			    emit this->requestActivateConnection(settings->path());
			    return;
		    }
		    qCInfo(
		        logNetworkManager
		    ) << "Failed to connectWithSettings: The provided settings no longer exist.";
	    }
	);

	// clang-format off
	QObject::connect(frontend, &Network::requestForget, this, &NMNetwork::forget);
	QObject::connect(frontend, &Network::requestDisconnect, this, &NMNetwork::requestDisconnect);
	QObject::connect(this, &NMNetwork::settingsAdded, frontend, &Network::settingsAdded);
	QObject::connect(this, &NMNetwork::settingsRemoved, frontend, &Network::settingsRemoved);
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
		if (auto* settingsRef = this->referenceSettings()) {
			emit this->requestActivateConnection(settingsRef->path());
			return;
		}
		emit this->requestAddAndActivateConnection(NMSettingsMap(), "/");
		return;
	});
}

NMWirelessNetwork::NMWirelessNetwork(const QString& ssid, NetworkDevice* device, QObject* parent)
    : NMNetwork(parent)
    , mSsid(ssid)
    , bSecurity(WifiSecurityType::Unknown) {

	auto updateSecurity = [this]() {
		if (NMSettings* settings = this->bReferenceSettings) {
			this->bSecurity.setBinding([settings]() { return securityFromSettingsMap(settings->map()); });
		} else if (NMAccessPoint* ap = this->bReferenceAp) {
			this->bSecurity.setBinding([ap]() { return ap->security(); });
		} else {
			this->bSecurity = WifiSecurityType::Unknown;
		}
		return;
	};

	auto checkDisappeared = [this]() {
		if (this->mAccessPoints.isEmpty() && this->mSettings.isEmpty()) emit this->disappeared();
	};

	QObject::connect(this, &NMWirelessNetwork::referenceSettingsChanged, this, updateSecurity);
	QObject::connect(this, &NMWirelessNetwork::referenceApChanged, this, updateSecurity);
	QObject::connect(this, &NMWirelessNetwork::settingsRemoved, this, checkDisappeared);
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
	if (this->mAccessPoints.contains(ap->path())) return;
	this->mAccessPoints.insert(ap->path(), ap);
	auto onDestroyed = [this, ap]() {
		if (this->mAccessPoints.take(ap->path())) {
			emit this->apRemoved(ap);
			this->updateReferenceAp();
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
		if (auto* settingsRef = this->referenceSettings()) {
			emit this->requestActivateConnection(settingsRef->path());
			return;
		}
		if (auto* apRef = this->referenceAp()) {
			emit this->requestAddAndActivateConnection(NMSettingsMap(), apRef->path());
			return;
		}
		emit this->requestAddAndActivateConnection(NMSettingsMap(), "/");
		return;
	});

	QObject::connect(frontend, &WifiNetwork::requestConnectWithPsk, this, [this](const QString& psk) {
		NMSettingsMap settings;
		settings["802-11-wireless-security"]["psk"] = psk;
		if (const QPointer<NMSettings> ref = this->referenceSettings()) {
			auto* call = ref->updateSettings(settings);
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
