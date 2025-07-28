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
#include "../frontend.hpp"
#include "nm/dbus_nm_backend.h"
#include "wireless.hpp"

namespace qs::network {

namespace {
Q_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

const QString NM_SERVICE = "org.freedesktop.NetworkManager";
const QString NM_PATH = "/org/freedesktop/NetworkManager";

NetworkManager::NetworkManager(QObject* parent): NetworkBackend(parent) {
	qCDebug(logNetworkManager) << "Starting NetworkManager Network Backend";

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
	QObject::connect(this->proxy, &DBusNetworkManagerProxy::DeviceAdded, this, &NetworkManager::onDeviceAdded);
	QObject::connect(this->proxy, &DBusNetworkManagerProxy::DeviceRemoved, this, &NetworkManager::onDeviceRemoved);
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
				this->queueDeviceRegistration(devicePath.path());
			}
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void NetworkManager::queueDeviceRegistration(const QString& path) {
	if (this->mDeviceHash.contains(path)) {
		qCDebug(logNetworkManager) << "Skipping duplicate registration of device" << path;
		return;
	}

	auto* deviceAdapter = new NMDeviceAdapter(path);

	if (!deviceAdapter->isValid()) {
		qCWarning(logNetworkManager) << "Ignoring invalid registration of" << path;
		delete deviceAdapter;
		return;
	}

	QObject::connect(
	    deviceAdapter,
	    &NMDeviceAdapter::ready,
	    this,
	    [this, deviceAdapter, path]() { this->registerDevice(deviceAdapter, path); },
	    Qt::SingleShotConnection
	);
}

NetworkDeviceState::Enum NetworkManager::toNetworkDeviceState(NMDeviceState::Enum state) {
	switch (state) {
	case 0 ... 20: return NetworkDeviceState::Unknown;
	case 30: return NetworkDeviceState::Disconnected;
	case 40 ... 90: return NetworkDeviceState::Connecting;
	case 100: return NetworkDeviceState::Connected;
	case 110 ... 120: return NetworkDeviceState::Disconnecting;
	}
}

void NetworkManager::registerDevice(NMDeviceAdapter* deviceAdapter, const QString& path) {
	NetworkDevice* device = nullptr;
	switch (deviceAdapter->type()) {
	case NMDeviceType::Wifi: device = this->bindWirelessDevice(deviceAdapter, path); break;
	default: device = new NetworkDevice(this); break;
	}
	deviceAdapter->setParent(device);

	// clang-format off
	QObject::connect(deviceAdapter, &NMDeviceAdapter::hwAddressChanged, device, &NetworkDevice::setAddress);
	QObject::connect(deviceAdapter, &NMDeviceAdapter::interfaceChanged, device, &NetworkDevice::setName);
	QObject::connect(deviceAdapter, &NMDeviceAdapter::stateChanged, device, [device](NMDeviceState::Enum state) { device->setState(NetworkManager::toNetworkDeviceState(state)); });
	QObject::connect(device, &NetworkDevice::requestDisconnect, deviceAdapter, &NMDeviceAdapter::disconnect);
	// clang-format on

	device->setAddress(deviceAdapter->hwAddress());
	device->setName(deviceAdapter->interface());
	device->setState(NetworkManager::toNetworkDeviceState(deviceAdapter->state()));

	this->mDeviceHash.insert(path, device);
	emit deviceAdded(device);

	qCDebug(logNetworkManager) << "Registered device" << path;
}

// Create a NetworkWifiDevice, NMWirelessManager, and connect the adapters
NetworkWifiDevice*
NetworkManager::bindWirelessDevice(NMDeviceAdapter* deviceAdapter, const QString& path) {
	auto* device = new NetworkWifiDevice(this);
	auto* wirelessAdapter = new NMWirelessAdapter(path, device);
	auto* manager = new NMWirelessManager(device);

	// clang-format off
	QObject::connect(wirelessAdapter, &NMWirelessAdapter::lastScanChanged, device, &NetworkWifiDevice::scanComplete);
	QObject::connect(device, &NetworkWifiDevice::requestScan, wirelessAdapter, &NMWirelessAdapter::scan);
	QObject::connect(wirelessAdapter, &NMWirelessAdapter::networkAdded, manager, &NMWirelessManager::networkAdded);
	QObject::connect(wirelessAdapter, &NMWirelessAdapter::networkRemoved, manager, &NMWirelessManager::networkRemoved);
	QObject::connect(deviceAdapter, &NMDeviceAdapter::connectionLoaded, manager, &NMWirelessManager::connectionLoaded);
	QObject::connect(deviceAdapter, &NMDeviceAdapter::connectionRemoved, manager, &NMWirelessManager::connectionRemoved);
	QObject::connect(manager, &NMWirelessManager::wifiNetworkAdded, device, &NetworkWifiDevice::wifiNetworkAdded);
	QObject::connect(manager, &NMWirelessManager::wifiNetworkRemoved, device, &NetworkWifiDevice::wifiNetworkRemoved);
	// clang-format on

	return device;
}

void NetworkManager::onDeviceAdded(const QDBusObjectPath& path) {
	this->queueDeviceRegistration(path.path());
}

void NetworkManager::onDeviceRemoved(const QDBusObjectPath& path) {
	auto iter = this->mDeviceHash.find(path.path());

	if (iter == this->mDeviceHash.end()) {
		qCWarning(logNetworkManager) << "NetworkManager backend sent removal signal for" << path.path()
		                             << "which is not registered.";
	} else {
		auto* device = iter.value();
		this->mDeviceHash.erase(iter);
		emit deviceRemoved(device);
		delete device;
		qCDebug(logNetworkManager) << "Device" << path.path() << "removed.";
	}
}

bool NetworkManager::isAvailable() const { return this->proxy && this->proxy->isValid(); };

} // namespace qs::network
