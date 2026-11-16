#pragma once

#include <qcontainerfwd.h>
#include <qlist.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/doc.hpp"
#include "../../dbus/properties.hpp"
#include "dbus_device.h"

namespace qs::services::fprint {

class FprintDevice: public QObject {
	Q_OBJECT;
	// clang-format off
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);
	Q_PROPERTY(qint32 numEnrollStages READ default NOTIFY numEnrollStagesChanged BINDABLE bindableNumEnrollStages);
	Q_PROPERTY(QString scanType READ default NOTIFY scanTypeChanged BINDABLE bindableScanType);
	Q_PROPERTY(bool isFingerPresent READ default NOTIFY isFingerPresentChanged BINDABLE bindableIsFingerPresent);
	Q_PROPERTY(bool isFingerNeeded READ default NOTIFY isFingerNeededChanged BINDABLE bindableIsFingerNeeded);
	Q_PROPERTY(QString user READ default WRITE setUser NOTIFY userChanged BINDABLE bindableUser);
	Q_PROPERTY(QList<QString> enrolledFingers READ default NOTIFY enrolledFingersChanged BINDABLE bindableEnrolledFingers);
	Q_PROPERTY(bool ready READ default NOTIFY readyChanged BINDABLE bindableReady);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("FprintDevices can only be acquired from FprintManager");

public:
	explicit FprintDevice(QObject* parent = nullptr);

	void init(const QString& path);

	Q_INVOKABLE void verifyStart(const QString& finger);
	Q_INVOKABLE void verifyStop();
	Q_INVOKABLE void enrollStart(const QString& finger);
	Q_INVOKABLE void enrollStop();
	Q_INVOKABLE void deleteFingers();
	Q_INVOKABLE void deleteFinger(const QString& finger);

	[[nodiscard]] QBindable<QString> bindableName() const { return &this->bName; }

	[[nodiscard]] QBindable<qint32> bindableNumEnrollStages() const {
		return &this->bNumEnrollStages;
	}

	[[nodiscard]] QBindable<QString> bindableScanType() const { return &this->bScanType; }

	[[nodiscard]] QBindable<bool> bindableIsFingerPresent() const { return &this->bIsFingerPresent; }

	[[nodiscard]] QBindable<bool> bindableIsFingerNeeded() const { return &this->bIsFingerNeeded; }

	[[nodiscard]] QBindable<bool> bindableReady() const { return &this->bReady; }

	[[nodiscard]] QBindable<QString> bindableUser() const { return &this->bUser; }
	void setUser(const QString& user);

	[[nodiscard]] QBindable<QList<QString>> bindableEnrolledFingers() const {
		return &this->bEnrolledFingers;
	}
	[[nodiscard]] QList<QString> enrolledFingers() const { return this->mEnrolledFingers; }

signals:
	QSDOC_HIDE void readyChanged();
	void nameChanged();
	void numEnrollStagesChanged();
	void scanTypeChanged();
	void isFingerPresentChanged();
	void isFingerNeededChanged();
	void userChanged();
	void enrolledFingersChanged();
	void fingerSelected(const QString& finger);
	void fingerMatched(const QString& finger);
	void verifyStatusChanged(const QString& result, bool done);
	void enrollStatusChanged(const QString& result, bool done);

private slots:
	void onGetAllFinished();

private:
	void registerFingers();
	void releaseClaim();
	void wrapAction(const std::function<void()>& action);

	QList<QString> mEnrolledFingers;
	bool mVerifying;
	bool mEnrolling;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, QString, bName, &FprintDevice::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, qint32, bNumEnrollStages, &FprintDevice::numEnrollStagesChanged);
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, QString, bScanType, &FprintDevice::scanTypeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, bool, bIsFingerPresent, &FprintDevice::isFingerPresentChanged);
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, bool, bIsFingerNeeded, &FprintDevice::isFingerNeededChanged);
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, QString, bUser, &FprintDevice::userChanged);
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, QList<QString>, bEnrolledFingers, &FprintDevice::enrolledFingersChanged);
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, bool, bReady, &FprintDevice::readyChanged);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(FprintDevice, deviceProperties);
	QS_DBUS_PROPERTY_BINDING(FprintDevice, pName, bName, deviceProperties, "name");
	QS_DBUS_PROPERTY_BINDING(FprintDevice, pNumEnrollStages, bNumEnrollStages, deviceProperties, "num-enroll-stages");
	QS_DBUS_PROPERTY_BINDING(FprintDevice, pScanType, bScanType, deviceProperties, "scan-type");
	QS_DBUS_PROPERTY_BINDING(FprintDevice, pIsFingerPresent, bIsFingerPresent, deviceProperties, "finger-present");
	QS_DBUS_PROPERTY_BINDING(FprintDevice, pIsFingerNeeded, bIsFingerNeeded, deviceProperties, "finger-needed");
	// clang-format on

	DBusFprintDevice* device = nullptr;
};

} // namespace qs::services::fprint
