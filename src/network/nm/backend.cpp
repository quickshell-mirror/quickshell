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

#include "../../dbus/bus.hpp"
#include "../../dbus/properties.hpp"
#include "../api.hpp"
#include "dbus_nm_backend.h"
#include "adapters.hpp"

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
				    << "Could not start NetworkManager. The NetworkManager backend will not work.";
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

	auto* device = new Device(this);
	auto* deviceAdapter = new NMDeviceAdapter(device);

	deviceAdapter->init(path);

	if (!deviceAdapter->isValid()) {
		qCWarning(logNetworkManager) << "Ignoring invalid Device registration of" << path;
		delete device;
		return;
	}
	
	device->setAddress(deviceAdapter->getHwAddress());
	device->setName(deviceAdapter->getInterface());
	QObject::connect(deviceAdapter, &NMDeviceAdapter::hwAddressChanged, device, &Device::setAddress);
	QObject::connect(deviceAdapter, &NMDeviceAdapter::interfaceChanged, device, &Device::setName);


	this->mDeviceHash.insert(path, device);
	this->mDevices.insertObject(device);
	qCDebug(logNetworkManager) << "Registered Device" << path;
}

void NetworkManager::onDeviceAdded(const QDBusObjectPath& path) {
	this->registerDevice(path.path());
}

void NetworkManager::onDeviceRemoved(const QDBusObjectPath& path) {
	auto iter = this->mDeviceHash.find(path.path());

	if (iter == this->mDeviceHash.end()) {
		qCWarning(logNetworkManager) << "NetworkManager backend sent removal signal for" << path.path()
		                             << "which is not registered.";
	} else {
		auto* device = iter.value();
		this->mDeviceHash.erase(iter);
		this->mDevices.removeObject(device);
		qCDebug(logNetworkManager) << "Device" << path.path() << "removed.";
	}
}

UntypedObjectModel* NetworkManager::devices() { return &this->mDevices; }
// WirelessDevice* NetworkManager::wifiDevice() { return &this->mWifi; }
bool NetworkManager::isAvailable() const { return this->proxy && this->proxy->isValid(); }

} // namespace qs::network
