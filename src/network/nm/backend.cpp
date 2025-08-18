#include "backend.hpp"

#include <qcontainerfwd.h>
#include <qdbusextratypes.h>
#include <qdbusservicewatcher.h>
#include <qhash.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "../network.hpp"
#include "../wifi.hpp"
#include "device.hpp"
#include "nm/dbus_nm_backend.h"
#include "wireless.hpp"

namespace qs::network {

namespace {
Q_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

const QString NM_SERVICE = "org.freedesktop.NetworkManager";
const QString NM_PATH = "/org/freedesktop/NetworkManager";

NetworkManager::NetworkManager(QObject* parent): NetworkBackend(parent) {
	auto bus = QDBusConnection::systemBus();
	if (!bus.isConnected()) {
		qCWarning(logNetworkManager
		) << "Could not connect to DBus. NetworkManager backend will not work.";
		return;
	}

	this->proxy = new DBusNetworkManagerProxy(NM_SERVICE, NM_PATH, bus, this);

	if (!this->proxy->isValid()) {
		qCDebug(logNetworkManager
		) << "NetworkManager service is not currently running. This network backend will not work";
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
	if (this->mDeviceHash.contains(path)) {
		qCDebug(logNetworkManager) << "Skipping duplicate registration of device" << path;
		return;
	}

	// Introspect to decide the device variant. (For now, only Wireless)
	auto* introspection = new QDBusInterface(
	    "org.freedesktop.NetworkManager",
	    path,
	    "org.freedesktop.DBus.Introspectable",
	    QDBusConnection::systemBus(),
	    this
	);

	auto pending = introspection->asyncCall("Introspect");
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this, path](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<QString> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager) << "Failed to introspect device: " << reply.error().message();
		} else {
			QXmlStreamReader xml(reply.value());

			while (!xml.atEnd() && !xml.hasError()) {
				xml.readNext();

				if (xml.isStartElement() && xml.name() == "interface") {
					QString name = xml.attributes().value("name").toString();
					if (name.startsWith("org.freedesktop.NetworkManager.Device.Wireless")) {
						this->registerWifiDevice(path);
						break;
					}
				}
			}
		}
		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

NetworkConnectionState::Enum NetworkManager::toNetworkDeviceState(NMDeviceState::Enum state) {
	switch (state) {
	case 0 ... 20: return NetworkConnectionState::Unknown;
	case 30: return NetworkConnectionState::Disconnected;
	case 40 ... 90: return NetworkConnectionState::Connecting;
	case 100: return NetworkConnectionState::Connected;
	case 110 ... 120: return NetworkConnectionState::Disconnecting;
	}
}

void NetworkManager::registerWifiDevice(const QString& path) {
	auto* wireless = new NMWirelessDevice(path);
	if (!wireless->isWirelessValid() || !wireless->isDeviceValid()) {
		qCWarning(logNetworkManager) << "Ignoring invalid registration of" << path;
		delete wireless;
		return;
	}

	auto* device = new WifiDevice(this);
	wireless->setParent(device);
	this->mDeviceHash.insert(path, device);

	// clang-format off
	QObject::connect(wireless, &NMWirelessDevice::interfaceChanged, device, &WifiDevice::setName);
	QObject::connect(wireless, &NMWirelessDevice::hwAddressChanged, device, &WifiDevice::setAddress);
	QObject::connect(wireless, &NMWirelessDevice::stateChanged, device, &WifiDevice::setNmState);
	QObject::connect(wireless, &NMWirelessDevice::stateChanged, device, [device](NMDeviceState::Enum state) { device->setState(qs::network::NetworkManager::toNetworkDeviceState(state));});
	QObject::connect(wireless, &NMWirelessDevice::lastScanChanged, device, &WifiDevice::scanComplete);
	QObject::connect(wireless, &NMWirelessDevice::addAndActivateConnection, this, &NetworkManager::addAndActivateConnection);
	QObject::connect(wireless, &NMWirelessDevice::activateConnection, this, &NetworkManager::activateConnection);
	QObject::connect(wireless, &NMWirelessDevice::wifiNetworkAdded, device, &WifiDevice::networkAdded);
	QObject::connect(wireless, &NMWirelessDevice::wifiNetworkRemoved, device, &WifiDevice::networkRemoved);
	QObject::connect(device, &WifiDevice::requestScan, wireless, &NMWirelessDevice::scan);
	// clang-format on

	emit wifiDeviceAdded(device);
}

void NetworkManager::onDevicePathAdded(const QDBusObjectPath& path) {
	this->registerDevice(path.path());
}

void NetworkManager::onDevicePathRemoved(const QDBusObjectPath& path) {
	auto iter = this->mDeviceHash.find(path.path());
	if (iter == this->mDeviceHash.end()) {
		qCWarning(logNetworkManager) << "NetworkManager sent removal signal for" << path.path()
		                             << "which is not registered.";
	} else {
		auto* device = iter.value();
		this->mDeviceHash.erase(iter);
		if (auto* wifi = qobject_cast<WifiDevice*>(device)) {
			emit wifiDeviceRemoved(wifi);
		};
		delete device;
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
			qCWarning(logNetworkManager)
			    << "Failed to request connection activation:" << reply.error().message();
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
			    << "Failed to start add and activate connection:" << reply.error().message();
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
