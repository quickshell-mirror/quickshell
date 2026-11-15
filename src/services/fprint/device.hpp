#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "dbus_device.h"

namespace qs::services::fprint {

class FprintDevice: public QObject {
	Q_OBJECT;
	// clang-format off
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);
	Q_PROPERTY(qreal numEnrollStages READ default NOTIFY numEnrollStagesChanged BINDABLE bindableNumEnrollStages);
	Q_PROPERTY(QString scanType READ default NOTIFY scanTypeChanged BINDABLE bindableScanType);
	Q_PROPERTY(bool isFingerPresent READ default NOTIFY isFingerPresentChanged BINDABLE bindableIsFingerPresent);
	Q_PROPERTY(bool isFingerNeeded READ default NOTIFY isFingerNeededChanged BINDABLE bindableIsFingerNeeded);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("FprintDevices can only be acquired from FprintManager");

public:
	explicit FprintDevice(QObject* parent = nullptr);

	void init(const QString& path);

	[[nodiscard]] QBindable<QString> bindableName() const { return &this->bName; }
	[[nodiscard]] QBindable<qreal> bindableNumEnrollStages() const { return &this->bNumEnrollStages; }
	[[nodiscard]] QBindable<QString> bindableScanType() const { return &this->bScanType; }
	[[nodiscard]] QBindable<bool> bindableIsFingerPresent() const { return &this->bIsFingerPresent; }
	[[nodiscard]] QBindable<bool> bindableIsFingerNeeded() const { return &this->bIsFingerNeeded; }

signals:
	void nameChanged();
	void numEnrollStagesChanged();
	void scanTypeChanged();
	void isFingerPresentChanged();
	void isFingerNeededChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, QString, bName, &FprintDevice::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, qint32, bNumEnrollStages, &FprintDevice::numEnrollStagesChanged);
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, QString, bScanType, &FprintDevice::scanTypeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, bool, bIsFingerPresent, &FprintDevice::isFingerPresentChanged);
	Q_OBJECT_BINDABLE_PROPERTY(FprintDevice, bool, bIsFingerNeeded, &FprintDevice::isFingerNeededChanged);

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
