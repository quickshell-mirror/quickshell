#include "device.hpp"

#include <QtDBus/qdbuspendingcall.h>
#include <QtDBus/qdbusreply.h>
#include <qcontainerfwd.h>
#include <qlist.h>
#include <qloggingcategory.h>
#include <qobject.h>

namespace qs::services::fprint {

namespace {
QS_LOGGING_CATEGORY(logFprintDevice, "quickshell.service.fprint.device", QtWarningMsg);
}

FprintDevice::FprintDevice(QObject* parent): QObject(parent) {}

void FprintDevice::init(const QString& path) {
	this->device =
	    new DBusFprintDevice("net.reactivated.Fprint", path, QDBusConnection::systemBus(), this);

	if (!this->device->isValid()) {
		qCWarning(logFprintDevice) << "Cannot create FprintDevice for" << path;
		return;
	}

	this->deviceProperties.setInterface(this->device);
	this->deviceProperties.updateAllViaGetAll();

	this->registerFingers();

	QObject::connect(this, &FprintDevice::userChanged, &FprintDevice::registerFingers);
}

void FprintDevice::registerFingers() {
	auto pending = this->device->ListEnrolledFingers(this->bUser);
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto reponseCallback = [this](QDBusPendingCallWatcher* call) {
		QDBusPendingReply<QList<QString>> reply = *call;

		if (reply.isError()) {
			qCWarning(logFprintDevice) << "Failed to get enrolled fingers:" << reply.error().message();
		} else {
			for (const QString& finger: reply.value()) {
				this->mEnrolledFingers.append(finger);
			}
			this->bEnrolledFingers = this->mEnrolledFingers;
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, reponseCallback);
}

void FprintDevice::setUser(const QString& user) { this->bUser = user; }

} // namespace qs::services::fprint
