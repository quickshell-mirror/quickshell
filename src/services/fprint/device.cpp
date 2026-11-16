#include "device.hpp"

#include <qcontainerfwd.h>
#include <qlist.h>
#include <qloggingcategory.h>
#include <qobject.h>

#include "../../dbus/properties.hpp"

using namespace qs::dbus;

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

	QObject::connect(
	    &this->deviceProperties,
	    &DBusPropertyGroup::getAllFinished,
	    this,
	    &FprintDevice::onGetAllFinished
	);

	this->deviceProperties.setInterface(this->device);
	this->deviceProperties.updateAllViaGetAll();

	this->registerFingers();

	QObject::connect(this, &FprintDevice::userChanged, &FprintDevice::registerFingers);
	QObject::connect(
	    this->device,
	    &DBusFprintDevice::VerifyFingerSelected,
	    this,
	    &FprintDevice::fingerSelected
	);
	QObject::connect(
	    this->device,
	    &DBusFprintDevice::VerifyFingerMatched,
	    this,
	    &FprintDevice::fingerMatched
	);
	QObject::connect(
	    this->device,
	    &DBusFprintDevice::VerifyStatus,
	    this,
	    &FprintDevice::verifyStatusChanged
	);
	QObject::connect(
	    this->device,
	    &DBusFprintDevice::EnrollStatus,
	    this,
	    &FprintDevice::enrollStatusChanged
	);
	QObject::connect(
	    this,
	    &FprintDevice::verifyStatusChanged,
	    [this](const QString& /*result*/, bool done) {
		    if (done && this->mVerifying) this->releaseClaim();
	    }
	);
	QObject::connect(
	    this,
	    &FprintDevice::enrollStatusChanged,
	    [this](const QString& /*result*/, bool done) {
		    if (done && this->mEnrolling) this->releaseClaim();
	    }
	);
}

void FprintDevice::onGetAllFinished() {
	qCDebug(logFprintDevice) << "FprintDevice" << this->device->path() << "ready.";
	this->bReady = true;
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

void FprintDevice::wrapAction(const std::function<void()>& action) {
	auto pending = this->device->Claim(this->bUser);
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto reponseCallback = [action](QDBusPendingCallWatcher* call) {
		QDBusPendingReply<void> reply = *call;

		if (reply.isError()) {
			qCWarning(logFprintDevice) << "Failed to claim device:" << reply.error().message();
		} else {
			action();

			delete call;
		}
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, reponseCallback);
}

void FprintDevice::releaseClaim() {
	auto pending = this->device->Release();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [](QDBusPendingCallWatcher* call) {
		QDBusPendingReply<void> reply = *call;

		if (reply.isError()) {
			qCWarning(logFprintDevice) << "Failed to release device:" << reply.error().message();
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void FprintDevice::setUser(const QString& user) { this->bUser = user; }

void FprintDevice::verifyStart(const QString& finger) {
	if (!this->bReady) {
		qCWarning(logFprintDevice) << "Device is not ready yet.";
		return;
	}

	this->wrapAction([this, finger]() {
		auto pending = this->device->VerifyStart(finger);
		auto* call = new QDBusPendingCallWatcher(pending, this);

		auto responseCallback = [this](QDBusPendingCallWatcher* call) {
			QDBusPendingReply<void> reply = *call;

			if (reply.isError()) {
				qCWarning(logFprintDevice) << "Failed to verify finger:" << reply.error().message();
			} else {
				this->mVerifying = true;
			}

			delete call;
		};

		QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
	});
}

void FprintDevice::verifyStop() {
	if (!this->bReady) {
		qCWarning(logFprintDevice) << "Device is not ready yet.";
		return;
	}

	if (!this->mVerifying) return;

	this->mVerifying = false;
	auto pending = this->device->VerifyStop();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [](QDBusPendingCallWatcher* call) {
		QDBusPendingReply<void> reply = *call;

		if (reply.isError()) {
			qCWarning(logFprintDevice) << "Failed to stop verification:" << reply.error().message();
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void FprintDevice::enrollStart(const QString& finger) {
	if (!this->bReady) {
		qCWarning(logFprintDevice) << "Device is not ready yet.";
		return;
	}

	this->wrapAction([this, finger]() {
		auto pending = this->device->EnrollStart(finger);
		auto* call = new QDBusPendingCallWatcher(pending, this);

		auto responseCallback = [this](QDBusPendingCallWatcher* call) {
			QDBusPendingReply<void> reply = *call;

			if (reply.isError()) {
				qCWarning(logFprintDevice) << "Failed to enroll finger:" << reply.error().message();
			} else {
				this->mEnrolling = true;
			}

			delete call;
		};

		QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
	});
}

void FprintDevice::enrollStop() {
	if (!this->bReady) {
		qCWarning(logFprintDevice) << "Device is not ready yet.";
		return;
	}

	if (!this->mEnrolling) return;

	this->mEnrolling = false;
	auto pending = this->device->EnrollStop();
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [](QDBusPendingCallWatcher* call) {
		QDBusPendingReply<void> reply = *call;

		if (reply.isError()) {
			qCWarning(logFprintDevice) << "Failed to stop enrollment:" << reply.error().message();
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void FprintDevice::deleteFingers() {
	if (!this->bReady) {
		qCWarning(logFprintDevice) << "Device is not ready yet.";
		return;
	}

	auto pending = this->device->DeleteEnrolledFingers(this->bUser);
	auto* call = new QDBusPendingCallWatcher(pending, this);

	auto responseCallback = [](QDBusPendingCallWatcher* call) {
		QDBusPendingReply<void> reply = *call;

		if (reply.isError()) {
			qCWarning(logFprintDevice) << "Failed to delete enrolled fingers:" << reply.error().message();
		}

		delete call;
	};

	QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
}

void FprintDevice::deleteFinger(const QString& finger) {
	if (!this->bReady) {
		qCWarning(logFprintDevice) << "Device is not ready yet.";
		return;
	}

	this->wrapAction([this, finger]() {
		auto pending = this->device->DeleteEnrolledFinger(finger);
		auto* call = new QDBusPendingCallWatcher(pending, this);

		auto responseCallback = [this](QDBusPendingCallWatcher* call) {
			QDBusPendingReply<void> reply = *call;

			if (reply.isError()) {
				qCWarning(logFprintDevice)
				    << "Failed to delete enrolled finger:" << reply.error().message();
			} else {
				this->releaseClaim();
			}

			delete call;
		};

		QObject::connect(call, &QDBusPendingCallWatcher::finished, this, responseCallback);
	});
}

} // namespace qs::services::fprint
