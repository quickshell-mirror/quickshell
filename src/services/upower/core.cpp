#include "core.hpp"

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qdbusconnectioninterface.h>
#include <qdbusextratypes.h>
#include <qdbuspendingcall.h>
#include <qdbuspendingreply.h>
#include <qdbusservicewatcher.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qqmllist.h>

#include "../../core/logcat.hpp"
#include "../../core/model.hpp"
#include "../../dbus/bus.hpp"
#include "../../dbus/properties.hpp"
#include "dbus_service.h"
#include "device.hpp"

namespace qs::service::upower {

namespace {
QS_LOGGING_CATEGORY(logUPower, "quickshell.service.upower", QtWarningMsg);
}

UPower::UPower() {
	qCDebug(logUPower) << "Starting UPower Service";

	auto bus = QDBusConnection::systemBus();

	if (!bus.isConnected()) {
		qCWarning(logUPower) << "Could not connect to DBus. UPower service will not work.";
		return;
	}

	this->service =
	    new DBusUPowerService("org.freedesktop.UPower", "/org/freedesktop/UPower", bus, this);

	if (!this->service->isValid()) {
		qCDebug(logUPower) << "UPower service is not currently running, attempting to start it.";

		dbus::tryLaunchService(this, bus, "org.freedesktop.UPower", [this](bool success) {
			if (success) {
				qCDebug(logUPower) << "Successfully launched UPower service.";
				this->init();
			} else {
				qCWarning(logUPower) << "Could not start UPower. The UPower service will not work.";
			}
		});
	} else {
		this->init();
	}
}

void UPower::init() {
	QObject::connect(this->service, &DBusUPowerService::DeviceAdded, this, &UPower::onDeviceAdded);

	QObject::connect(
	    this->service,
	    &DBusUPowerService::DeviceRemoved,
	    this,
	    &UPower::onDeviceRemoved
	);

	this->serviceProperties.setInterface(this->service);
	this->serviceProperties.updateAllViaGetAll();

	this->registerDisplayDevice();
	this->registerDevices();
}

void UPower::registerDevices() {
	auto pending = this->service->EnumerateDevices();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<QList<QDBusObjectPath>> reply = *call;

		if (reply.isError()) {
			qCWarning(logUPower) << "Failed to enumerate devices:" << reply.error().message();
		} else {
			for (const QDBusObjectPath& devicePath: reply.value()) {
				this->registerDevice(devicePath.path());
			}
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void UPower::registerDisplayDevice() {
	auto pending = this->service->GetDisplayDevice();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<QDBusObjectPath> reply = *call;

		if (reply.isError()) {
			qCWarning(logUPower) << "Failed to get default device:" << reply.error().message();
		} else {
			qCDebug(logUPower) << "UPower default device registered at" << reply.value().path();
			this->mDisplayDevice.init(reply.value().path());
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void UPower::onDeviceAdded(const QDBusObjectPath& path) { this->registerDevice(path.path()); }

void UPower::onDeviceRemoved(const QDBusObjectPath& path) {
	auto iter = this->mDevices.find(path.path());

	if (iter == this->mDevices.end()) {
		qCWarning(logUPower) << "UPower service sent removal signal for" << path.path()
		                     << "which is not registered.";
	} else {
		auto* device = iter.value();
		this->mDevices.erase(iter);
		this->readyDevices.removeObject(device);
		qCDebug(logUPower) << "UPowerDevice" << device->path() << "removed.";
	}
}

void UPower::onDeviceReady() {
	auto* device = qobject_cast<UPowerDevice*>(this->sender());

	this->readyDevices.insertObject(device);
}

void UPower::registerDevice(const QString& path) {
	if (this->mDevices.contains(path)) {
		qCDebug(logUPower) << "Skipping duplicate registration of UPowerDevice" << path;
		return;
	}

	auto* device = new UPowerDevice(this);
	device->init(path);

	if (!device->isValid()) {
		qCWarning(logUPower) << "Ignoring invalid UPowerDevice registration of" << path;
		delete device;
		return;
	}

	this->mDevices.insert(path, device);
	QObject::connect(device, &UPowerDevice::readyChanged, this, &UPower::onDeviceReady);

	qCDebug(logUPower) << "Registered UPowerDevice" << path;
}

UPowerDevice* UPower::displayDevice() { return &this->mDisplayDevice; }

ObjectModel<UPowerDevice>* UPower::devices() { return &this->readyDevices; }

UPower* UPower::instance() {
	static UPower* instance = new UPower(); // NOLINT
	return instance;
}

UPowerQml::UPowerQml(QObject* parent): QObject(parent) {
	QObject::connect(
	    UPower::instance(),
	    &UPower::onBatteryChanged,
	    this,
	    &UPowerQml::onBatteryChanged
	);
}

UPowerDevice* UPowerQml::displayDevice() { // NOLINT
	return UPower::instance()->displayDevice();
}

ObjectModel<UPowerDevice>* UPowerQml::devices() { // NOLINT
	return UPower::instance()->devices();
}

} // namespace qs::service::upower
