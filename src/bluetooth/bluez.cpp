#include "bluez.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qdbusextratypes.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include "../core/logcat.hpp"
#include "../dbus/dbus_objectmanager_types.hpp"
#include "../dbus/objectmanager.hpp"
#include "adapter.hpp"
#include "device.hpp"

namespace qs::bluetooth {

namespace {
QS_LOGGING_CATEGORY(logBluetooth, "quickshell.bluetooth", QtWarningMsg);
}

Bluez* Bluez::instance() {
	static auto* instance = new Bluez();
	return instance;
}

Bluez::Bluez() { this->init(); }

void Bluez::updateDefaultAdapter() {
	const auto& adapters = this->mAdapters.valueList();
	this->bDefaultAdapter = adapters.empty() ? nullptr : adapters.first();
}

void Bluez::init() {
	qCDebug(logBluetooth) << "Connecting to BlueZ";

	auto bus = QDBusConnection::systemBus();

	if (!bus.isConnected()) {
		qCWarning(logBluetooth) << "Could not connect to DBus. Bluetooth integration is not available.";
		return;
	}

	this->objectManager = new qs::dbus::DBusObjectManager(this);

	QObject::connect(
	    this->objectManager,
	    &qs::dbus::DBusObjectManager::interfacesAdded,
	    this,
	    &Bluez::onInterfacesAdded
	);

	QObject::connect(
	    this->objectManager,
	    &qs::dbus::DBusObjectManager::interfacesRemoved,
	    this,
	    &Bluez::onInterfacesRemoved
	);

	if (!this->objectManager->setInterface("org.bluez", "/", bus)) {
		qCDebug(logBluetooth) << "BlueZ is not running. Bluetooth integration will not work.";
		return;
	}
}

void Bluez::onInterfacesAdded(
    const QDBusObjectPath& path,
    const DBusObjectManagerInterfaces& interfaces
) {
	if (auto* adapter = this->mAdapterMap.value(path.path())) {
		for (const auto& [interface, properties]: interfaces.asKeyValueRange()) {
			adapter->addInterface(interface, properties);
		}
	} else if (auto* device = this->mDeviceMap.value(path.path())) {
		for (const auto& [interface, properties]: interfaces.asKeyValueRange()) {
			device->addInterface(interface, properties);
		}
	} else if (interfaces.contains("org.bluez.Adapter1")) {
		auto* adapter = new BluetoothAdapter(path.path(), this);

		if (!adapter->isValid()) {
			qCWarning(logBluetooth) << "Adapter path is not valid, cannot track: " << device;
			delete adapter;
			return;
		}

		qCDebug(logBluetooth) << "Tracked new adapter" << adapter;

		for (const auto& [interface, properties]: interfaces.asKeyValueRange()) {
			adapter->addInterface(interface, properties);
		}

		for (auto* device: this->mDevices.valueList()) {
			if (device->adapterPath() == path) {
				adapter->devices()->insertObject(device);
				qCDebug(logBluetooth) << "Added tracked device" << device << "to new adapter" << adapter;
				emit device->adapterChanged();
			}
		}

		this->mAdapterMap.insert(path.path(), adapter);
		this->mAdapters.insertObject(adapter);
		this->updateDefaultAdapter();
	} else if (interfaces.contains("org.bluez.Device1")) {
		auto* device = new BluetoothDevice(path.path(), this);

		if (!device->isValid()) {
			qCWarning(logBluetooth) << "Device path is not valid, cannot track: " << device;
			delete device;
			return;
		}

		qCDebug(logBluetooth) << "Tracked new device" << device;

		for (const auto& [interface, properties]: interfaces.asKeyValueRange()) {
			device->addInterface(interface, properties);
		}

		if (auto* adapter = device->adapter()) {
			adapter->devices()->insertObject(device);
			qCDebug(logBluetooth) << "Added device" << device << "to adapter" << adapter;
		}

		this->mDeviceMap.insert(path.path(), device);
		this->mDevices.insertObject(device);
	}
}

void Bluez::onInterfacesRemoved(const QDBusObjectPath& path, const QStringList& interfaces) {
	if (auto* adapter = this->mAdapterMap.value(path.path())) {
		if (interfaces.contains("org.bluez.Adapter1")) {
			qCDebug(logBluetooth) << "Adapter removed:" << adapter;

			this->mAdapterMap.remove(path.path());
			this->mAdapters.removeObject(adapter);
			this->updateDefaultAdapter();
			delete adapter;
		}
	} else if (auto* device = this->mDeviceMap.value(path.path())) {
		if (interfaces.contains("org.bluez.Device1")) {
			qCDebug(logBluetooth) << "Device removed:" << device;

			if (auto* adapter = device->adapter()) {
				adapter->devices()->removeObject(device);
			}

			this->mDeviceMap.remove(path.path());
			this->mDevices.removeObject(device);
			delete device;
		} else {
			for (const auto& interface: interfaces) {
				device->removeInterface(interface);
			}
		}
	}
}

BluezQml::BluezQml() {
	QObject::connect(
	    Bluez::instance(),
	    &Bluez::defaultAdapterChanged,
	    this,
	    &BluezQml::defaultAdapterChanged
	);
}

} // namespace qs::bluetooth
