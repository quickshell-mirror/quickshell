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
#include <qxmlstream.h>

#include "../../core/logcat.hpp"
#include "../../dbus/properties.hpp"
#include "../device.hpp"
#include "../network.hpp"
#include "../wifi.hpp"
#include "dbus_types.hpp"
#include "enums.hpp"
#include "nm/dbus_nm_backend.h"
#include "wireless.hpp"

namespace qs::network {

namespace {
QS_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

NetworkManager::NetworkManager(QObject* parent): NetworkBackend(parent) {
	qDBusRegisterMetaType<ConnectionSettingsMap>();

	auto bus = QDBusConnection::systemBus();
	if (!bus.isConnected()) {
		qCWarning(logNetworkManager
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
		qCDebug(logNetworkManager
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

	auto responseCallback = [this, path, introspection](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<QString> reply = *call;

		if (reply.isError()) {
			qCWarning(logNetworkManager) << "Failed to introspect device: " << reply.error().message();
		} else {
			QXmlStreamReader xml(reply.value());

			while (!xml.atEnd() && !xml.hasError()) {
				xml.readNext();

				if (xml.isStartElement() && xml.name() == "interface") {
					const QString name = xml.attributes().value("name").toString();
					if (name.startsWith("org.freedesktop.NetworkManager.Device.Wireless")) {
						this->registerWifiDevice(path);
						break;
					}
				}
			}
		}
		delete call;
		delete introspection;
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

	device->bindableName().setBinding([wireless]() { return wireless->interface(); });
	device->bindableAddress().setBinding([wireless]() { return wireless->hwAddress(); });
	device->bindableNmState().setBinding([wireless]() { return wireless->state(); });
	device->bindableState().setBinding([wireless]() {
		return qs::network::NetworkManager::toNetworkDeviceState(wireless->state());
	});
	device->bindableScanning().setBinding([wireless]() { return wireless->scanning(); });
	// clang-format off
	QObject::connect(wireless, &NMWirelessDevice::addAndActivateConnection, this, &NetworkManager::addAndActivateConnection);
	QObject::connect(wireless, &NMWirelessDevice::activateConnection, this, &NetworkManager::activateConnection);
	QObject::connect(wireless, &NMWirelessDevice::wifiNetworkAdded, device, &WifiDevice::networkAdded);
	QObject::connect(wireless, &NMWirelessDevice::wifiNetworkRemoved, device, &WifiDevice::networkRemoved);
	QObject::connect(device, &WifiDevice::requestScan, wireless, &NMWirelessDevice::scan);
	QObject::connect(device, &WifiDevice::requestDisconnect, wireless, &NMWirelessDevice::disconnect);
	// clang-format on

	emit this->wifiDeviceAdded(device);
}

void NetworkManager::onDevicePathAdded(const QDBusObjectPath& path) {
	this->registerDevice(path.path());
}

void NetworkManager::onDevicePathRemoved(const QDBusObjectPath& path) {
	auto iter = this->mDeviceHash.find(path.path());
	if (iter == this->mDeviceHash.end()) {
		qCWarning(logNetworkManager) << "Sent removal signal for" << path.path()
		                             << "which is not registered.";
	} else {
		auto* device = iter.value();
		this->mDeviceHash.erase(iter);
		if (auto* wifi = qobject_cast<WifiDevice*>(device)) emit this->wifiDeviceRemoved(wifi);
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
