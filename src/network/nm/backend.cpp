#include "backend.hpp"

#include <qdbusconnection.h>
#include <qdbusextratypes.h>
#include <qdbusmetatype.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/logcat.hpp"
#include "../../dbus/properties.hpp"
#include "../device.hpp"
#include "../network.hpp"
#include "../wifi.hpp"
#include "dbus_nm_backend.h"
#include "dbus_nm_device.h"
#include "device.hpp"
#include "enums.hpp"
#include "types.hpp"
#include "wireless.hpp"

namespace qs::network {

namespace {
QS_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

NetworkManager::NetworkManager(QObject* parent): NetworkBackend(parent) {
	qDBusRegisterMetaType<ConnectionSettingsMap>();

	auto bus = QDBusConnection::systemBus();
	if (!bus.isConnected()) {
		qCWarning(
		    logNetworkManager
		) << "Could not connect to DBus. NetworkManager backend will not work.";
		return;
	}

	this->proxy = new DBusNetworkManagerProxy(
	    "org.freedesktop.NetworkManager",
	    "/org/freedesktop/NetworkManager",
	    bus,
	    this
	);

	if (!this->proxy->isValid()) {
		qCDebug(
		    logNetworkManager
		) << "NetworkManager is not currently running. This network backend will not work";
	} else {
		this->init();
	}
}

void NetworkManager::init() {
	// clang-format off
	QObject::connect(this->proxy, &DBusNetworkManagerProxy::DeviceAdded, this, &NetworkManager::onDevicePathAdded);
	QObject::connect(this->proxy, &DBusNetworkManagerProxy::DeviceRemoved, this, &NetworkManager::onDevicePathRemoved);
	// clang-format on

	this->dbusProperties.setInterface(this->proxy);
	this->dbusProperties.updateAllViaGetAll();

	this->registerDevices();
}

void NetworkManager::registerDevices() {
	auto pending = this->proxy->GetAllDevices();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<QList<QDBusObjectPath>> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager) << "Failed to get devices: " << reply.error().message();
		} else {
			for (const QDBusObjectPath& devicePath: reply.value()) {
				this->registerDevice(devicePath.path());
			}
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void NetworkManager::registerDevice(const QString& path) {
	if (this->mDevices.contains(path)) {
		qCDebug(logNetworkManager) << "Skipping duplicate registration of device" << path;
		return;
	}

	auto* temp = new DBusNMDeviceProxy(
	    "org.freedesktop.NetworkManager",
	    path,
	    QDBusConnection::systemBus(),
	    this
	);

	auto callback = [this, path, temp](uint value, const QDBusError& error) {
		if (error.isValid()) {
			qCWarning(logNetworkManager) << "Failed to get device type:" << error;
		} else {
			auto type = static_cast<qs::network::NMDeviceType::Enum>(value);
			NMDevice* dev = nullptr;
			this->mDevices.insert(path, nullptr);

			switch (type) {
			case NMDeviceType::Wifi: dev = new NMWirelessDevice(path); break;
			default: break;
			}

			if (dev) {
				if (!dev->isValid()) {
					qCWarning(logNetworkManager) << "Ignoring invalid registration of" << path;
					delete dev;
				} else {
					this->mDevices[path] = dev;
					// clang-format off
					QObject::connect(dev, &NMDevice::addAndActivateConnection, this, &NetworkManager::addAndActivateConnection);
					QObject::connect(dev, &NMDevice::activateConnection, this, &NetworkManager::activateConnection);
					// clang-format on

					this->registerFrontendDevice(type, dev);
				}
			}
			temp->deleteLater();
		}
	};

	qs::dbus::asyncReadProperty<uint>(*temp, "DeviceType", callback);
}

void NetworkManager::registerFrontendDevice(NMDeviceType::Enum type, NMDevice* dev) {
	NetworkDevice* frontendDev = nullptr;
	switch (type) {
	case NMDeviceType::Wifi: {
		auto* frontendWifiDev = new WifiDevice(dev);
		auto* wifiDev = qobject_cast<NMWirelessDevice*>(dev);
		// Bind WifiDevice-specific properties
		auto translateMode = [wifiDev]() {
			switch (wifiDev->mode()) {
			case NM80211Mode::Unknown: return WifiDeviceMode::Unknown;
			case NM80211Mode::Adhoc: return WifiDeviceMode::AdHoc;
			case NM80211Mode::Infra: return WifiDeviceMode::Station;
			case NM80211Mode::Ap: return WifiDeviceMode::AccessPoint;
			case NM80211Mode::Mesh: return WifiDeviceMode::Mesh;
			}
		};
		// clang-format off
		frontendWifiDev->bindableMode().setBinding(translateMode);
		wifiDev->bindableScanning().setBinding([frontendWifiDev]() { return frontendWifiDev->scannerEnabled(); });
		QObject::connect(wifiDev, &NMWirelessDevice::networkAdded, frontendWifiDev, &WifiDevice::networkAdded);
		QObject::connect(wifiDev, &NMWirelessDevice::networkRemoved, frontendWifiDev, &WifiDevice::networkRemoved);
		// clang-format on
		frontendDev = frontendWifiDev;
		break;
	}
	default: return;
	}

	// Bind generic NetworkDevice properties
	auto translateState = [dev]() {
		switch (dev->state()) {
		case 0 ... 20: return DeviceConnectionState::Unknown;
		case 30: return DeviceConnectionState::Disconnected;
		case 40 ... 90: return DeviceConnectionState::Connecting;
		case 100: return DeviceConnectionState::Connected;
		case 110 ... 120: return DeviceConnectionState::Disconnecting;
		}
	};
	// clang-format off
	frontendDev->bindableName().setBinding([dev]() { return dev->interface(); });
	frontendDev->bindableAddress().setBinding([dev]() { return dev->hwAddress(); });
	frontendDev->bindableNmState().setBinding([dev]() { return dev->state(); });
	frontendDev->bindableState().setBinding(translateState);
	frontendDev->bindableAutoconnect().setBinding([dev]() { return dev->autoconnect(); });
	frontendDev->bindableNmManaged().setBinding([dev]() { return dev->managed(); });
	QObject::connect(frontendDev, &WifiDevice::requestDisconnect, dev, &NMDevice::disconnect);
	QObject::connect(frontendDev, &NetworkDevice::requestSetAutoconnect, dev, &NMDevice::setAutoconnect);
	QObject::connect(frontendDev, &NetworkDevice::requestSetNmManaged, dev, &NMDevice::setManaged);
	// clang-format on

	this->mFrontendDevices.insert(dev->path(), frontendDev);
	emit this->deviceAdded(frontendDev);
}

void NetworkManager::removeFrontendDevice(NMDevice* dev) {
	auto* frontendDev = this->mFrontendDevices.take(dev->path());
	if (frontendDev) {
		emit this->deviceRemoved(frontendDev);
		frontendDev->deleteLater();
	}
}

void NetworkManager::onDevicePathAdded(const QDBusObjectPath& path) {
	this->registerDevice(path.path());
}

void NetworkManager::onDevicePathRemoved(const QDBusObjectPath& path) {
	auto iter = this->mDevices.find(path.path());
	if (iter == this->mDevices.end()) {
		qCWarning(logNetworkManager) << "Sent removal signal for" << path.path()
		                             << "which is not registered.";
	} else {
		auto* dev = iter.value();
		this->mDevices.erase(iter);
		if (dev) {
			this->removeFrontendDevice(dev);
			delete dev;
		}
	}
}

void NetworkManager::activateConnection(
    const QDBusObjectPath& connPath,
    const QDBusObjectPath& devPath
) {
	auto pending = this->proxy->ActivateConnection(connPath, devPath, QDBusObjectPath("/"));
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<QDBusObjectPath> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager) << "Failed to activate connection:" << reply.error().message();
		}
		delete call;
	};
	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void NetworkManager::addAndActivateConnection(
    const ConnectionSettingsMap& settings,
    const QDBusObjectPath& devPath,
    const QDBusObjectPath& specificObjectPath
) {
	auto pending = this->proxy->AddAndActivateConnection(settings, devPath, specificObjectPath);
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<QDBusObjectPath, QDBusObjectPath> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager)
			    << "Failed to add and activate connection:" << reply.error().message();
		}
		delete call;
	};
	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void NetworkManager::setWifiEnabled(bool enabled) {
	if (enabled == this->bWifiEnabled) return;
	this->bWifiEnabled = enabled;
	this->pWifiEnabled.write();
}

bool NetworkManager::isAvailable() const { return this->proxy && this->proxy->isValid(); };

} // namespace qs::network
