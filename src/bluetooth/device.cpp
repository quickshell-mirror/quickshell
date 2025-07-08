#include "device.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qstring.h>
#include <qstringliteral.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/logcat.hpp"
#include "../dbus/properties.hpp"
#include "adapter.hpp"
#include "bluez.hpp"
#include "dbus_device.h"

namespace qs::bluetooth {

namespace {
QS_LOGGING_CATEGORY(logDevice, "quickshell.bluetooth.device", QtWarningMsg);
}

QString BluetoothDeviceState::toString(BluetoothDeviceState::Enum state) {
	switch (state) {
	case BluetoothDeviceState::Disconnected: return QStringLiteral("Disconnected");
	case BluetoothDeviceState::Connected: return QStringLiteral("Connected");
	case BluetoothDeviceState::Disconnecting: return QStringLiteral("Disconnecting");
	case BluetoothDeviceState::Connecting: return QStringLiteral("Connecting");
	default: return QStringLiteral("Unknown");
	}
}

BluetoothDevice::BluetoothDevice(const QString& path, QObject* parent): QObject(parent) {
	this->mInterface =
	    new DBusBluezDeviceInterface("org.bluez", path, QDBusConnection::systemBus(), this);

	if (!this->mInterface->isValid()) {
		qCWarning(logDevice) << "Could not create DBus interface for device at" << path;
		delete this->mInterface;
		this->mInterface = nullptr;
		return;
	}

	this->properties.setInterface(this->mInterface);
}

BluetoothAdapter* BluetoothDevice::adapter() const {
	return Bluez::instance()->adapter(this->bAdapterPath.value().path());
}

void BluetoothDevice::setConnected(bool connected) {
	if (connected == this->bConnected) return;

	if (connected) {
		this->connect();
	} else {
		this->disconnect();
	}
}

void BluetoothDevice::setTrusted(bool trusted) {
	if (trusted == this->bTrusted) return;
	this->bTrusted = trusted;
	this->pTrusted.write();
}

void BluetoothDevice::setBlocked(bool blocked) {
	if (blocked == this->bBlocked) return;
	this->bBlocked = blocked;
	this->pBlocked.write();
}

void BluetoothDevice::setName(const QString& name) {
	if (name == this->bName) return;
	this->bName = name;
	this->pName.write();
}

void BluetoothDevice::setWakeAllowed(bool wakeAllowed) {
	if (wakeAllowed == this->bWakeAllowed) return;
	this->bWakeAllowed = wakeAllowed;
	this->pWakeAllowed.write();
}

void BluetoothDevice::connect() {
	if (this->bConnected) {
		qCCritical(logDevice) << "Device" << this << "is already connected";
		return;
	}

	if (this->bState == BluetoothDeviceState::Connecting) {
		qCCritical(logDevice) << "Device" << this << "is already connecting";
		return;
	}

	qCDebug(logDevice) << "Connecting to device" << this;
	this->bState = BluetoothDeviceState::Connecting;

	auto reply = this->mInterface->Connect();
	auto* watcher = new QDBusPendingCallWatcher(reply, this);

	QObject::connect(
	    watcher,
	    &QDBusPendingCallWatcher::finished,
	    this,
	    [this](QDBusPendingCallWatcher* watcher) {
		    const QDBusPendingReply<> reply = *watcher;

		    if (reply.isError()) {
			    qCWarning(logDevice).nospace()
			        << "Failed to connect to device " << this << ": " << reply.error().message();

			    this->bState = this->bConnected ? BluetoothDeviceState::Connected
			                                    : BluetoothDeviceState::Disconnected;
		    } else {
			    qCDebug(logDevice) << "Successfully connected to to device" << this;
		    }

		    delete watcher;
	    }
	);
}

void BluetoothDevice::disconnect() {
	if (!this->bConnected) {
		qCCritical(logDevice) << "Device" << this << "is already disconnected";
		return;
	}

	if (this->bState == BluetoothDeviceState::Disconnecting) {
		qCCritical(logDevice) << "Device" << this << "is already disconnecting";
		return;
	}

	qCDebug(logDevice) << "Disconnecting from device" << this;
	this->bState = BluetoothDeviceState::Disconnecting;

	auto reply = this->mInterface->Disconnect();
	auto* watcher = new QDBusPendingCallWatcher(reply, this);

	QObject::connect(
	    watcher,
	    &QDBusPendingCallWatcher::finished,
	    this,
	    [this](QDBusPendingCallWatcher* watcher) {
		    const QDBusPendingReply<> reply = *watcher;

		    if (reply.isError()) {
			    qCWarning(logDevice).nospace()
			        << "Failed to disconnect from device " << this << ": " << reply.error().message();

			    this->bState = this->bConnected ? BluetoothDeviceState::Connected
			                                    : BluetoothDeviceState::Disconnected;
		    } else {
			    qCDebug(logDevice) << "Successfully disconnected from from device" << this;
		    }

		    delete watcher;
	    }
	);
}

void BluetoothDevice::pair() {
	if (this->bPaired) {
		qCCritical(logDevice) << "Device" << this << "is already paired";
		return;
	}

	if (this->bPairing) {
		qCCritical(logDevice) << "Device" << this << "is already pairing";
		return;
	}

	qCDebug(logDevice) << "Pairing with device" << this;
	this->bPairing = true;

	auto reply = this->mInterface->Pair();
	auto* watcher = new QDBusPendingCallWatcher(reply, this);

	QObject::connect(
	    watcher,
	    &QDBusPendingCallWatcher::finished,
	    this,
	    [this](QDBusPendingCallWatcher* watcher) {
		    const QDBusPendingReply<> reply = *watcher;
		    if (reply.isError()) {
			    qCWarning(logDevice).nospace()
			        << "Failed to pair with device " << this << ": " << reply.error().message();
		    } else {
			    qCDebug(logDevice) << "Successfully initiated pairing with device" << this;
		    }

		    this->bPairing = false;
		    delete watcher;
	    }
	);
}

void BluetoothDevice::cancelPair() {
	if (!this->bPairing) {
		qCCritical(logDevice) << "Device" << this << "is not currently pairing";
		return;
	}

	qCDebug(logDevice) << "Cancelling pairing with device" << this;

	auto reply = this->mInterface->CancelPairing();
	auto* watcher = new QDBusPendingCallWatcher(reply, this);

	QObject::connect(
	    watcher,
	    &QDBusPendingCallWatcher::finished,
	    this,
	    [this](QDBusPendingCallWatcher* watcher) {
		    const QDBusPendingReply<> reply = *watcher;
		    if (reply.isError()) {
			    qCWarning(logDevice) << "Failed to cancel pairing with device" << this << ":"
			                         << reply.error().message();
		    } else {
			    qCDebug(logDevice) << "Successfully cancelled pairing with device" << this;
		    }

		    this->bPairing = false;
		    delete watcher;
	    }
	);
}

void BluetoothDevice::forget() {
	if (!this->mInterface || !this->mInterface->isValid()) {
		qCCritical(logDevice) << "Cannot forget - device interface is invalid";
		return;
	}

	if (auto* adapter = Bluez::instance()->adapter(this->bAdapterPath.value().path())) {
		qCDebug(logDevice) << "Forgetting device" << this << "via adapter" << adapter;
		adapter->removeDevice(this->path());
	} else {
		qCCritical(logDevice) << "Could not find adapter for path" << this->bAdapterPath.value().path()
		                      << "to forget from";
	}
}

void BluetoothDevice::addInterface(const QString& interface, const QVariantMap& properties) {
	if (interface == "org.bluez.Device1") {
		this->properties.updatePropertySet(properties, false);
		qCDebug(logDevice) << "Updated Device properties for" << this;
	} else if (interface == "org.bluez.Battery1") {
		if (!this->mBatteryInterface) {
			this->mBatteryInterface = new QDBusInterface(
			    "org.bluez",
			    this->path(),
			    "org.bluez.Battery1",
			    QDBusConnection::systemBus(),
			    this
			);

			if (!this->mBatteryInterface->isValid()) {
				qCWarning(logDevice) << "Could not create Battery interface for device at" << this;
				delete this->mBatteryInterface;
				this->mBatteryInterface = nullptr;
				return;
			}
		}

		this->batteryProperties.setInterface(this->mBatteryInterface);
		this->batteryProperties.updatePropertySet(properties, false);

		emit this->batteryAvailableChanged();
		qCDebug(logDevice) << "Updated Battery properties for" << this;
	}
}

void BluetoothDevice::removeInterface(const QString& interface) {
	if (interface == "org.bluez.Battery1" && this->mBatteryInterface) {
		this->batteryProperties.setInterface(nullptr);
		delete this->mBatteryInterface;
		this->mBatteryInterface = nullptr;
		this->bBattery = 0;

		emit this->batteryAvailableChanged();
		qCDebug(logDevice) << "Battery interface removed from device" << this;
	}
}

void BluetoothDevice::onConnectedChanged() {
	this->bState =
	    this->bConnected ? BluetoothDeviceState::Connected : BluetoothDeviceState::Disconnected;
	emit this->connectedChanged();
}

} // namespace qs::bluetooth

namespace qs::dbus {

using namespace qs::bluetooth;

DBusResult<qreal> DBusDataTransform<BatteryPercentage>::fromWire(quint8 percentage) {
	return DBusResult(percentage * 0.01);
}

} // namespace qs::dbus

QDebug operator<<(QDebug debug, const qs::bluetooth::BluetoothDevice* device) {
	auto saver = QDebugStateSaver(debug);

	if (device) {
		debug.nospace() << "BluetoothDevice(" << static_cast<const void*>(device)
		                << ", path=" << device->path() << ")";
	} else {
		debug << "BluetoothDevice(nullptr)";
	}

	return debug;
}
