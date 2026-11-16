#pragma once

#include <functional>

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

///! A fingerprint reader device.
class FprintDevice: public QObject {
	Q_OBJECT;
	// clang-format off
	/// The product name of the device.
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);
	/// The number of enrollment stages for the device.
	/// [!NOTE] This is only available when the device has been claimed, otherwise it will be undefined (-1).
	/// See @@FprintDevice.user and @@FprintDevice.enrollStart
	Q_PROPERTY(qint32 numEnrollStages READ default NOTIFY numEnrollStagesChanged BINDABLE bindableNumEnrollStages);
	/// The scan type of the device, either "press" if you place your finger on the device, or "swipe" if you have to swipe your finger.
	Q_PROPERTY(QString scanType READ default NOTIFY scanTypeChanged BINDABLE bindableScanType);
	/// Whether the finger is on sensor.
	Q_PROPERTY(bool fingerPresent READ default NOTIFY fingerPresentChanged BINDABLE bindableFingerPresent);
	/// Whether the sensor is waiting for the finger.
	Q_PROPERTY(bool fingerNeeded READ default NOTIFY fingerNeededChanged BINDABLE bindableFingerNeeded);
	/// The user who currently claims the device.
	Q_PROPERTY(QString user READ default WRITE setUser NOTIFY userChanged BINDABLE bindableUser);
	/// The enrolled fingerprints on the device for the current @@FprintDevice.user. This will be empty if the user is null.
	Q_PROPERTY(QList<QString> enrolledFingers READ default NOTIFY enrolledFingersChanged BINDABLE bindableEnrolledFingers);
	/// If device statistics have been queried for this device yet. This will be true for all devices returned from @@FprintManager.devices, but not the default device, which may be returned before it is ready to avoid returning null.
	Q_PROPERTY(bool ready READ default NOTIFY ready BINDABLE bindableReady);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("FprintDevices can only be acquired from FprintManager");

public:
	explicit FprintDevice(QObject* parent = nullptr);

	void init(const QString& path);

	/// Check the chosen finger against a saved fingerprint.
	/// > [!NOTE] You must claim the device via @@FprintDevice.user.
	/// Valid finger names for this method:
	/// > left-thumb: Left thumb
	/// > left-index-finger: Left index finger
	/// > left-middle-finger: Left middle finger
	/// > left-ring-finger: Left ring finger
	/// > left-little-finger: Left little finger
	/// > right-thumb: Right thumb
	/// > right-index-finger: Right index finger
	/// > right-middle-finger: Right middle finger
	/// > right-ring-finger: Right ring finger
	/// > right-little-finger: Right little finger
	/// > any: Any finger, only used for @@FprintDevice.verifyStart (select the first finger with a fingerprint associated) and @@FprintDevice.fingerSelected
	/// The selected finger is sent to @@FprintDevice.fingerSelected and the verification status through @@FprintDevice.verifyStatusChanged
	Q_INVOKABLE void verifyStart(const QString& finger);
	/// Stop an on-going fingerprint verification started with @@FprintDevice.verifyStart.
	Q_INVOKABLE void verifyStop();
	/// Start enrollment for the selected finger.
	/// > [!NOTE] You must claim the device via @@FprintDevice.user.
	/// Enrollment status is sent through @@FprintDevice.enrollStatusChanged.
	/// > [!NOTE] "any" is not a valid finger name for this method.
	/// > See @@FprintDevice.verifyStart for valid finger names.
	Q_INVOKABLE void enrollStart(const QString& finger);
	/// Stop an on-going fingerprint enrollment started with @@FprintDevice.enrollStart.
	Q_INVOKABLE void enrollStop();
	/// Delete all the enrolled fingerprints for the current @@FprintDevice.user.
	Q_INVOKABLE void deleteFingers();
	/// Delete the enrolled fingerprint for the current @@FprintDevice.user.
	/// > [!NOTE] "any" is not a valid finger name for this method.
	/// > See @@FprintDevice.verifyStart for valid finger names.
	Q_INVOKABLE void deleteFinger(const QString& finger);

	[[nodiscard]] QBindable<QString> bindableName() const { return &this->bName; }

	[[nodiscard]] QBindable<qint32> bindableNumEnrollStages() const {
		return &this->bNumEnrollStages;
	}

	[[nodiscard]] QBindable<QString> bindableScanType() const { return &this->bScanType; }

	[[nodiscard]] QBindable<bool> bindableFingerPresent() const { return &this->bFingerPresent; }

	[[nodiscard]] QBindable<bool> bindableFingerNeeded() const { return &this->bFingerNeeded; }

	[[nodiscard]] QBindable<bool> bindableReady() const { return &this->bReady; }

	[[nodiscard]] QBindable<QString> bindableUser() const { return &this->bUser; }
	void setUser(const QString& user);

	[[nodiscard]] QBindable<QList<QString>> bindableEnrolledFingers() const {
		return &this->bEnrolledFingers;
	}
	[[nodiscard]] QList<QString> enrolledFingers() const { return this->mEnrolledFingers; }

signals:
	QSDOC_HIDE void ready();
	void nameChanged();
	void numEnrollStagesChanged();
	void scanTypeChanged();
	void fingerPresentChanged();
	void fingerNeededChanged();
	void userChanged();
	void enrolledFingersChanged();

	/// The selected finger to be verified.
	/// > [!NOTE] See @@FprintDevice.verifyStart for finger names.
	void fingerSelected(const QString& finger);
	/// The finger that has been verified.
	/// > [!NOTE] See @@FprintDevice.verifyStart for finger names.
	void fingerMatched(const QString& finger);
	/// Called when the verification status changes.
	/// The *result* can be one of:
	/// > verify-no-match: The verification did not match.
	/// > verify-match: The verification succeeded.
	/// > verify-retry-scan: The user should retry scanning their finger, the verification is still ongoing.
	/// > verify-swipe-too-short: The user's swipe was too short. The user should retry scanning their finger, the verification is still ongoing.
	/// > verify-finger-not-centered: The user's finger was not centered on the reader. The user should retry scanning their finger, the verification is still ongoing.
	/// > verify-remove-and-retry: The user should remove their finger from the scanner and retry scanning their finger, the verification is still ongoing.
	/// > verify-disconnected: The device was disconnected during the verification, no other actions should be taken, the device shouldn't be used anymore.
	/// > verify-unknown-error: An unknown error occured (usually a driver problem).
	void verifyStatusChanged(const QString& result, bool done);
	/// Called when the enrollment status changes.
	/// The *result* can be one of:
	/// >  enroll-completed: The enrollment successfully completed.
	/// > enroll-failed: The enrollment failed.
	/// > enroll-stage-passed: One stage of the enrollment passed, the enrollment is still ongoing.
	/// > enroll-retry-scan: The user should retry scanning their finger, the enrollment is still ongoing.
	/// > enroll-swipe-too-short: The user's swipe was too short. The user should retry scanning their finger, the enrollment is still ongoing.
	/// > enroll-too-fast: The user's swipe or touch was too short. The user should retry scanning their finger, the enrollment is still ongoing.
	/// > enroll-finger-not-centered: The user's finger was not centered on the reader. The user should retry scanning their finger, the enrollment is still ongoing.
	/// > enroll-remove-and-retry: The user should remove their finger from the reader and retry scanning their finger, the enrollment is still ongoing.
	/// > enroll-data-full: No further prints can be enrolled on this device. Delete other prints from the device first to continue.
	/// > enroll-duplicate: The print has already been enrolled. The user should enroll a different finger, or delete the print that has been enrolled already.
	/// > enroll-disconnected: The device was disconnected during the enrollment, no other actions should be taken, and you shouldn't use the device any more.
	/// > enroll-unknown-error: An unknown error occurred (usually a driver problem)
	void enrollStatusChanged(const QString& result, bool done);

private slots:
	void onGetAllFinished();

private:
	void registerFingers();
	void releaseClaim();
	void wrapAction(const std::function<void()>& action);

	QList<QString> mEnrolledFingers;
	bool mVerifying {false};
	bool mEnrolling {false};

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, QString, bName, &FprintDevice::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, qint32, bNumEnrollStages, &FprintDevice::numEnrollStagesChanged);
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, QString, bScanType, &FprintDevice::scanTypeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, bool, bFingerPresent, &FprintDevice::fingerPresentChanged);
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, bool, bFingerNeeded, &FprintDevice::fingerNeededChanged);
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, QString, bUser, &FprintDevice::userChanged);
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, QList<QString>, bEnrolledFingers, &FprintDevice::enrolledFingersChanged);
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, bool, bReady, &FprintDevice::ready);

	QS_DBUS_BINDABLE_PROPERTY_GROUP(FprintDevice, deviceProperties);
	QS_DBUS_PROPERTY_BINDING(FprintDevice, pName, bName, deviceProperties, "name");
	QS_DBUS_PROPERTY_BINDING(FprintDevice, pNumEnrollStages, bNumEnrollStages, deviceProperties, "num-enroll-stages");
	QS_DBUS_PROPERTY_BINDING(FprintDevice, pScanType, bScanType, deviceProperties, "scan-type");
	QS_DBUS_PROPERTY_BINDING(FprintDevice, pFingerPresent, bFingerPresent, deviceProperties, "finger-present");
	QS_DBUS_PROPERTY_BINDING(FprintDevice, pFingerNeeded, bFingerNeeded, deviceProperties, "finger-needed");
	// clang-format on

	DBusFprintDevice* device = nullptr;
};

} // namespace qs::services::fprint
