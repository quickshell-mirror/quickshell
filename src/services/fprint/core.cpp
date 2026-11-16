#include "core.hpp"

#include <QtDBus/qdbuspendingcall.h>
#include <QtDBus/qdbuspendingreply.h>
#include <qobject.h>

#include "device.hpp"

namespace qs::services::fprint {

namespace {
QS_LOGGING_CATEGORY(logFprint, "quickshell.service.fprint", QtWarningMsg);
}

FprintManager::FprintManager() {
	this->service = new DBusFprintService(
	    "net.reactivated.Fprint",
	    "/net/reactivated/Fprint/Manager",
	    QDBusConnection::systemBus(),
	    this
	);

	this->init();
}

FprintManager* FprintManager::instance() {
	static FprintManager* fprintManager = new FprintManager(); // NOLINT
	return fprintManager;
}

void FprintManager::init() {
	this->registerDevices();
	this->registerDefaultDevice();
}

void FprintManager::registerDevices() {
	auto pending = this->service->GetDevices();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<QList<QDBusObjectPath>> reply = *call;

		if (reply.isError()) {
			qCWarning(logFprint) << "Failed to get devices:" << reply.error().message();
		} else {
			for (const QDBusObjectPath& devicePath: reply.value()) {
				this->registerDevice(devicePath.path());
			}
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void FprintManager::registerDefaultDevice() {
	auto pending = this->service->GetDefaultDevice();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [this](QDBusPendingCallWatcher* call) {
		const QDBusPendingReply<QDBusObjectPath> reply = *call;

		if (reply.isError()) {
			qCWarning(logFprint) << "Failed to get default device:" << reply.error().message();
		} else {
			this->mDefaultDevice.init(reply.value().path());
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void FprintManager::registerDevice(const QString& path) {
	auto* device = new FprintDevice(this);
	device->init(path);

	this->mDevices.insert(path, device);
	QObject::connect(device, &FprintDevice::readyChanged, this, &FprintManager::onDeviceReady);
}

void FprintManager::onDeviceReady() {
	auto* device = qobject_cast<FprintDevice*>(this->sender());
	this->mReadyDevices.insertObject(device);
}

ObjectModel<FprintDevice>* FprintManager::devices() { return &this->mReadyDevices; }

FprintDevice* FprintManager::defaultDevice() { return &this->mDefaultDevice; }

FprintManagerQml::FprintManagerQml(QObject* parent): QObject(parent) {}

ObjectModel<FprintDevice>* FprintManagerQml::devices() // NOLINT
{
	return FprintManager::instance()->devices();
}

FprintDevice* FprintManagerQml::defaultDevice() // NOLINT
{
	return FprintManager::instance()->defaultDevice();
}

} // namespace qs::services::fprint
