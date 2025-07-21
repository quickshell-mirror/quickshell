#include "nm_backend.hpp"

#include <qcontainerfwd.h>
#include <qdbusextratypes.h>
#include <qdbusservicewatcher.h>
#include <qhash.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../dbus/bus.hpp"
#include "../dbus/properties.hpp"
#include "api.hpp"
#include "dbus_nm_backend.h"
#include "nm_adapters.hpp"

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
		) << "NetworkManager service is not currently running, attempting to start it.";

		dbus::tryLaunchService(this, bus, NM_SERVICE, [this](bool success) {
			if (success) {
				qCDebug(logNetworkManager) << "Successfully launched NetworkManager backend.";
				this->init();
			} else {
				qCWarning(logNetworkManager)
				    << "Could not start NetworkManager. This network backend will not work.";
			}
		});
	} else {
		this->init();
	}
}

void NetworkManager::init() {
	QObject::connect(
	    this->proxy,
	    &DBusNetworkManagerProxy::DeviceAdded,
	    this,
	    &NetworkManager::onDeviceAdded
	);
	QObject::connect(
	    this->proxy,
	    &DBusNetworkManagerProxy::DeviceRemoved,
	    this,
	    &NetworkManager::onDeviceRemoved
	);

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

	// Wait to receive NMDeviceType before registering device
	QObject::connect(
	    deviceAdapter,
	    &NMDeviceAdapter::typeChanged,
	    this,
	    [this, deviceAdapter, path](NMDeviceType::Enum type) {
		    this->registerDevice(deviceAdapter, type, path);
	    },
	    Qt::SingleShotConnection
	);
}

void NetworkManager::registerDevice(
    NMDeviceAdapter* deviceAdapter,
    NMDeviceType::Enum type,
    const QString& path
) {
	NetworkDevice* device = createDeviceVariant(type, path);
	deviceAdapter->setParent(device);

	// NMDeviceAdapter signal -> NetworkDevice slot
	QObject::connect(
	    deviceAdapter,
	    &NMDeviceAdapter::hwAddressChanged,
	    device,
	    &NetworkDevice::setAddress
	);
	QObject::connect(
	    deviceAdapter,
	    &NMDeviceAdapter::interfaceChanged,
	    device,
	    &NetworkDevice::setName
	);
	QObject::connect(
	    deviceAdapter,
	    &NMDeviceAdapter::stateChanged,
	    device,
	    [device](NMDeviceState::Enum state) {
		    device->setState(NMDeviceState::toNetworkDeviceState(state));
	    }
	);

	// NetworkDevice signal -> NMDeviceAdapter slot
	QObject::connect(
	    device,
	    &NetworkDevice::signalDisconnect,
	    deviceAdapter,
	    &NMDeviceAdapter::disconnect
	);

	this->mDeviceHash.insert(path, device);
	emit deviceAdded(device);

	qCDebug(logNetworkManager) << "Registered device" << path;
}

// Create a device derived from NMDeviceType
NetworkDevice* NetworkManager::createDeviceVariant(NMDeviceType::Enum type, const QString& path) {
	switch (type) {
	case NMDeviceType::Wifi: return this->bindWirelessDevice(path);
	default: return new NetworkDevice();
	}
}

// Create a WirelessNetworkDevice and connect the NMWirelessAdapter
NetworkWifiDevice* NetworkManager::bindWirelessDevice(const QString& path) {
	auto* device = new NetworkWifiDevice(this);
	auto* wirelessAdapter = new NMWirelessAdapter(path, device);

	// TODO: Check isValid() - throw error

	// NMWirelessAdapter signal -> WirelessNetworkDevice slot
	QObject::connect(
	    wirelessAdapter,
	    &NMWirelessAdapter::lastScanChanged,
	    device,
	    &NetworkWifiDevice::scanComplete
	);
	QObject::connect(
	    wirelessAdapter,
	    &NMWirelessAdapter::wifiNetworkAdded,
	    device,
	    &NetworkWifiDevice::addNetwork
	);
	QObject::connect(
	    wirelessAdapter,
	    &NMWirelessAdapter::wifiNetworkRemoved,
	    device,
	    &NetworkWifiDevice::removeNetwork
	);

	// WirelessNetworkDevice signal -> NMWirelessAdapter slot
	QObject::connect(
	    device,
	    &NetworkWifiDevice::signalScan,
	    wirelessAdapter,
	    &NMWirelessAdapter::scan
	);

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

bool NetworkManager::isAvailable() const { return this->proxy && this->proxy->isValid(); }

} // namespace qs::network
