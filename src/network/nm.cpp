#include "nm.hpp"

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
#include "dbus_nm_service.h"

namespace qs::network {

namespace {
Q_LOGGING_CATEGORY(logNetworkManager, "quickshell.service.networkmanager", QtWarningMsg);
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

	this->service = new DBusNetworkManager(NM_SERVICE, NM_PATH, bus, this);

	if (!this->service->isValid()) {
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
	    this->service,
	    &DBusNetworkManager::DeviceAdded,
	    this,
	    &NetworkManager::onDeviceAdded
	);

	QObject::connect(
	    this->service,
	    &DBusNetworkManager::DeviceRemoved,
	    this,
	    &NetworkManager::onDeviceRemoved
	);

	this->serviceProperties.setInterface(this->service);
	this->serviceProperties.updateAllViaGetAll();

	this->registerDevices();
}

void NetworkManager::registerDevices() {
	auto pending = this->service->GetAllDevices();
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
		qCDebug(logNetworkManager) << "Skipping duplicate registration of NMDevice" << path;
		return;
	}

	auto* device = new NMDevice(this);
	device->init(path);

	if (!device->isValid()) {
		qCWarning(logNetworkManager) << "Ignoring invalid NMDevice registration of" << path;
		delete device;
		return;
	}

	this->mDeviceHash.insert(path, device);
	this->mDevices.insertObject(device);
	qCDebug(logNetworkManager) << "Registered NMDevice" << path;
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
		qCDebug(logNetworkManager) << "NMDevice" << device->path() << "removed.";
	}
}

UntypedObjectModel* NetworkManager::allDevices() { return &this->mDevices; }
NMDevice* NetworkManager::wireless() { return &this->mWireless; }
bool NetworkManager::isAvailable() const { return this->service && this->service->isValid(); }

} // namespace qs::network
