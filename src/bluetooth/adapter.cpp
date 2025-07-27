#include "adapter.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qdbusextratypes.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qstring.h>
#include <qstringliteral.h>
#include <qtypes.h>

#include "../core/logcat.hpp"
#include "../dbus/properties.hpp"
#include "dbus_adapter.h"

namespace qs::bluetooth {

namespace {
QS_LOGGING_CATEGORY(logAdapter, "quickshell.bluetooth.adapter", QtWarningMsg);
}

QString BluetoothAdapterState::toString(BluetoothAdapterState::Enum state) {
	switch (state) {
	case BluetoothAdapterState::Disabled: return QStringLiteral("Disabled");
	case BluetoothAdapterState::Enabled: return QStringLiteral("Enabled");
	case BluetoothAdapterState::Enabling: return QStringLiteral("Enabling");
	case BluetoothAdapterState::Disabling: return QStringLiteral("Disabling");
	case BluetoothAdapterState::Blocked: return QStringLiteral("Blocked");
	default: return QStringLiteral("Unknown");
	}
}

BluetoothAdapter::BluetoothAdapter(const QString& path, QObject* parent): QObject(parent) {
	this->mInterface =
	    new DBusBluezAdapterInterface("org.bluez", path, QDBusConnection::systemBus(), this);

	if (!this->mInterface->isValid()) {
		qCWarning(logAdapter) << "Could not create DBus interface for adapter at" << path;
		this->mInterface = nullptr;
		return;
	}

	this->properties.setInterface(this->mInterface);
}

QString BluetoothAdapter::adapterId() const {
	auto path = this->path();
	return path.sliced(path.lastIndexOf('/') + 1);
}

void BluetoothAdapter::setEnabled(bool enabled) {
	if (enabled == this->bEnabled) return;

	if (enabled && this->bState == BluetoothAdapterState::Blocked) {
		qCCritical(logAdapter) << "Cannot enable adapter because it is blocked by rfkill.";
		return;
	}

	this->bEnabled = enabled;
	this->pEnabled.write();
}

void BluetoothAdapter::setDiscoverable(bool discoverable) {
	if (discoverable == this->bDiscoverable) return;
	this->bDiscoverable = discoverable;
	this->pDiscoverable.write();
}

void BluetoothAdapter::setDiscovering(bool discovering) {
	if (discovering) {
		this->startDiscovery();
	} else {
		this->stopDiscovery();
	}
}

void BluetoothAdapter::setDiscoverableTimeout(quint32 timeout) {
	if (timeout == this->bDiscoverableTimeout) return;
	this->bDiscoverableTimeout = timeout;
	this->pDiscoverableTimeout.write();
}

void BluetoothAdapter::setPairable(bool pairable) {
	if (pairable == this->bPairable) return;
	this->bPairable = pairable;
	this->pPairable.write();
}

void BluetoothAdapter::setPairableTimeout(quint32 timeout) {
	if (timeout == this->bPairableTimeout) return;
	this->bPairableTimeout = timeout;
	this->pPairableTimeout.write();
}

void BluetoothAdapter::addInterface(const QString& interface, const QVariantMap& properties) {
	if (interface == "org.bluez.Adapter1") {
		this->properties.updatePropertySet(properties, false);
		qCDebug(logAdapter) << "Updated Adapter properties for" << this;
	}
}

void BluetoothAdapter::removeDevice(const QString& devicePath) {
	qCDebug(logAdapter) << "Removing device" << devicePath << "from adapter" << this;

	auto reply = this->mInterface->RemoveDevice(QDBusObjectPath(devicePath));

	auto* watcher = new QDBusPendingCallWatcher(reply, this);

	QObject::connect(
	    watcher,
	    &QDBusPendingCallWatcher::finished,
	    this,
	    [this, devicePath](QDBusPendingCallWatcher* watcher) {
		    const QDBusPendingReply<> reply = *watcher;

		    if (reply.isError()) {
			    qCWarning(logAdapter).nospace()
			        << "Failed to remove device " << devicePath << " from adapter" << this << ": "
			        << reply.error().message();
		    } else {
			    qCDebug(logAdapter) << "Successfully removed device" << devicePath << "from adapter"
			                        << this;
		    }

		    delete watcher;
	    }
	);
}

void BluetoothAdapter::startDiscovery() {
	if (this->bDiscovering) return;
	qCDebug(logAdapter) << "Starting discovery for adapter" << this;

	auto reply = this->mInterface->StartDiscovery();
	auto* watcher = new QDBusPendingCallWatcher(reply, this);

	QObject::connect(
	    watcher,
	    &QDBusPendingCallWatcher::finished,
	    this,
	    [this](QDBusPendingCallWatcher* watcher) {
		    const QDBusPendingReply<> reply = *watcher;

		    if (reply.isError()) {
			    qCWarning(logAdapter).nospace()
			        << "Failed to start discovery on adapter" << this << ": " << reply.error().message();
		    } else {
			    qCDebug(logAdapter) << "Successfully started discovery on adapter" << this;
		    }

		    delete watcher;
	    }
	);
}

void BluetoothAdapter::stopDiscovery() {
	if (!this->bDiscovering) return;
	qCDebug(logAdapter) << "Stopping discovery for adapter" << this;

	auto reply = this->mInterface->StopDiscovery();
	auto* watcher = new QDBusPendingCallWatcher(reply, this);

	QObject::connect(
	    watcher,
	    &QDBusPendingCallWatcher::finished,
	    this,
	    [this](QDBusPendingCallWatcher* watcher) {
		    const QDBusPendingReply<> reply = *watcher;

		    if (reply.isError()) {
			    qCWarning(logAdapter).nospace()
			        << "Failed to stop discovery on adapter " << this << ": " << reply.error().message();
		    } else {
			    qCDebug(logAdapter) << "Successfully stopped discovery on adapter" << this;
		    }

		    delete watcher;
	    }
	);
}

} // namespace qs::bluetooth

namespace qs::dbus {

using namespace qs::bluetooth;

DBusResult<BluetoothAdapterState::Enum>
DBusDataTransform<BluetoothAdapterState::Enum>::fromWire(const Wire& wire) {
	if (wire == QStringLiteral("off")) {
		return BluetoothAdapterState::Disabled;
	} else if (wire == QStringLiteral("on")) {
		return BluetoothAdapterState::Enabled;
	} else if (wire == QStringLiteral("off-enabling")) {
		return BluetoothAdapterState::Enabling;
	} else if (wire == QStringLiteral("on-disabling")) {
		return BluetoothAdapterState::Disabling;
	} else if (wire == QStringLiteral("off-blocked")) {
		return BluetoothAdapterState::Blocked;
	} else {
		return QDBusError(
		    QDBusError::InvalidArgs,
		    QString("Invalid BluetoothAdapterState: %1").arg(wire)
		);
	}
}

} // namespace qs::dbus

QDebug operator<<(QDebug debug, const qs::bluetooth::BluetoothAdapter* adapter) {
	auto saver = QDebugStateSaver(debug);

	if (adapter) {
		debug.nospace() << "BluetoothAdapter(" << static_cast<const void*>(adapter)
		                << ", path=" << adapter->path() << ")";
	} else {
		debug << "BluetoothAdapter(nullptr)";
	}

	return debug;
}
