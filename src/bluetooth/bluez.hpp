#pragma once

#include <qcontainerfwd.h>
#include <qhash.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

#include "../core/doc.hpp"
#include "../core/model.hpp"
#include "../dbus/dbus_objectmanager_types.hpp"
#include "../dbus/objectmanager.hpp"

namespace qs::bluetooth {

class BluetoothAdapter;
class BluetoothDevice;

class Bluez: public QObject {
	Q_OBJECT;

public:
	[[nodiscard]] ObjectModel<BluetoothAdapter>* adapters() { return &this->mAdapters; }
	[[nodiscard]] ObjectModel<BluetoothDevice>* devices() { return &this->mDevices; }
	[[nodiscard]] BluetoothAdapter* defaultAdapter() const;

	[[nodiscard]] BluetoothAdapter* adapter(const QString& path) {
		return this->mAdapterMap.value(path);
	}

	static Bluez* instance();

private slots:
	void
	onInterfacesAdded(const QDBusObjectPath& path, const DBusObjectManagerInterfaces& interfaces);
	void onInterfacesRemoved(const QDBusObjectPath& path, const QStringList& interfaces);

private:
	explicit Bluez();
	void init();

	qs::dbus::DBusObjectManager* objectManager = nullptr;
	QHash<QString, BluetoothAdapter*> mAdapterMap;
	QHash<QString, BluetoothDevice*> mDeviceMap;
	ObjectModel<BluetoothAdapter> mAdapters {this};
	ObjectModel<BluetoothDevice> mDevices {this};
};

///! Bluetooth manager
/// Provides access to bluetooth devices and adapters.
class BluezQml: public QObject {
	Q_OBJECT;
	/// The default bluetooth adapter. Usually there is only one.
	Q_PROPERTY(BluetoothAdapter* defaultAdapter READ defaultAdapter CONSTANT);
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::bluetooth::BluetoothAdapter>*);
	/// A list of all bluetooth adapters. See @@defaultAdapter for the default.
	Q_PROPERTY(UntypedObjectModel* adapters READ adapters CONSTANT);
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::bluetooth::BluetoothDevice>*);
	/// A list of all connected bluetooth devices across all adapters.
	/// See @@BluetoothAdapter.devices for the devices connected to a single adapter.
	Q_PROPERTY(UntypedObjectModel* devices READ devices CONSTANT);
	QML_NAMED_ELEMENT(Bluetooth);
	QML_SINGLETON;

public:
	explicit BluezQml(QObject* parent = nullptr): QObject(parent) {}

	[[nodiscard]] static ObjectModel<BluetoothAdapter>* adapters() {
		return Bluez::instance()->adapters();
	}

	[[nodiscard]] static ObjectModel<BluetoothDevice>* devices() {
		return Bluez::instance()->devices();
	}

	[[nodiscard]] static BluetoothAdapter* defaultAdapter() {
		return Bluez::instance()->defaultAdapter();
	}
};

} // namespace qs::bluetooth
