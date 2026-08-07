#include "backend.hpp"

#include <qbytearray.h>
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
#include "../qml.hpp"
#include "dbus_nm_backend.h"
#include "dbus_nm_device.h"
#include "dbus_types.hpp"
#include "device.hpp"
#include "enums.hpp"
#include "wired.hpp"
#include "wireless.hpp"

namespace qs::network {

namespace {
QS_LOGGING_CATEGORY(logNetworkManager, "quickshell.network.networkmanager", QtWarningMsg);
}

NetworkManager::NetworkManager(QObject* parent): NetworkBackend(parent) {
	qCDebug(logNetworkManager) << "Connecting to NetworkManager";
	qDBusRegisterMetaType<NMSettingsMap>();

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

void NetworkManager::checkConnectivity() {
	auto pending = this->proxy->CheckConnectivity();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<quint32> reply = *call;

		if (reply.isError()) {
			qCInfo(logNetworkManager) << "Failed to check connectivity: " << reply.error().message();
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
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
			case NMDeviceType::Ethernet: dev = new NMWiredDevice(path); break;
			default: break;
			}

			if (dev) {
				qCDebug(logNetworkManager) << "Device added:" << path;
				if (!dev->isValid()) {
					qCWarning(logNetworkManager) << "Ignoring invalid registration of" << path;
					delete dev;
				} else {
					this->mDevices[path] = dev;
					// clang-format off
					QObject::connect(dev, &NMDevice::addAndActivateConnection, this, &NetworkManager::addAndActivateConnection);
					QObject::connect(dev, &NMDevice::activateConnection, this, &NetworkManager::activateConnection);
					// clang-format on
					QObject::connect(dev, &NMDevice::loaded, this, [this, dev]() {
						emit this->deviceAdded(dev->frontend());
					});
				}
			} else {
				qCDebug(logNetworkManager) << "Ignoring registration of unsupported device:" << path;
			}
			temp->deleteLater();
		}
	};

	qs::dbus::asyncReadProperty<uint>(*temp, "DeviceType", callback);
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
			qCDebug(logNetworkManager) << "Device removed:" << path.path();
			emit this->deviceRemoved(dev->frontend());
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
    const NMSettingsMap& settings,
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

void NetworkManager::setConnectivityCheckEnabled(bool enabled) {
	if (enabled == this->bConnectivityCheckEnabled) return;
	this->bConnectivityCheckEnabled = enabled;
	this->pConnectivityCheckEnabled.write();
}

void NetworkManager::setWifiEnabled(bool enabled) {
	if (enabled == this->bWifiEnabled) return;
	this->bWifiEnabled = enabled;
	this->pWifiEnabled.write();
}

bool NetworkManager::isAvailable() const { return this->proxy && this->proxy->isValid(); };

} // namespace qs::network

namespace qs::dbus {

DBusResult<qs::network::NMConnectivityState::Enum>
DBusDataTransform<qs::network::NMConnectivityState::Enum>::fromWire(quint32 wire) {
	return DBusResult(static_cast<qs::network::NMConnectivityState::Enum>(wire));
}

} // namespace qs::dbus
