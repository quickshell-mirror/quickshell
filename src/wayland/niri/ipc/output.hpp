#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

namespace qs::niri::ipc {

class NiriIpc;

///! Niri output/monitor.
/// Represents a physical output as reported by niri.
class NiriOutput: public QObject {
	Q_OBJECT;
	// clang-format off
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);
	Q_PROPERTY(bool connected READ default NOTIFY connectedChanged BINDABLE bindableConnected);
	Q_PROPERTY(qreal scale READ default NOTIFY scaleChanged BINDABLE bindableScale);
	Q_PROPERTY(qint32 width READ default NOTIFY widthChanged BINDABLE bindableWidth);
	Q_PROPERTY(qint32 height READ default NOTIFY heightChanged BINDABLE bindableHeight);
	Q_PROPERTY(qint32 x READ default NOTIFY xChanged BINDABLE bindableX);
	Q_PROPERTY(qint32 y READ default NOTIFY yChanged BINDABLE bindableY);
	Q_PROPERTY(qint32 physicalWidth READ default NOTIFY physicalWidthChanged BINDABLE bindablePhysicalWidth);
	Q_PROPERTY(qint32 physicalHeight READ default NOTIFY physicalHeightChanged BINDABLE bindablePhysicalHeight);
	Q_PROPERTY(qreal refreshRate READ default NOTIFY refreshRateChanged BINDABLE bindableRefreshRate);
	Q_PROPERTY(bool vrrSupported READ default NOTIFY vrrSupportedChanged BINDABLE bindableVrrSupported);
	Q_PROPERTY(bool vrrEnabled READ default NOTIFY vrrEnabledChanged BINDABLE bindableVrrEnabled);
	Q_PROPERTY(QString transform READ default NOTIFY transformChanged BINDABLE bindableTransform);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("NiriOutputs must be retrieved from the Niri object.");

public:
	explicit NiriOutput(QObject* parent): QObject(parent) {}

	void updateFromJson(const QJsonObject& object);

	[[nodiscard]] QBindable<QString> bindableName() { return &this->bName; }
	[[nodiscard]] QBindable<bool> bindableConnected() { return &this->bConnected; }
	[[nodiscard]] QBindable<qreal> bindableScale() { return &this->bScale; }
	[[nodiscard]] QBindable<qint32> bindableWidth() { return &this->bWidth; }
	[[nodiscard]] QBindable<qint32> bindableHeight() { return &this->bHeight; }
	[[nodiscard]] QBindable<qint32> bindableX() { return &this->bX; }
	[[nodiscard]] QBindable<qint32> bindableY() { return &this->bY; }
	[[nodiscard]] QBindable<qint32> bindablePhysicalWidth() { return &this->bPhysicalWidth; }
	[[nodiscard]] QBindable<qint32> bindablePhysicalHeight() { return &this->bPhysicalHeight; }
	[[nodiscard]] QBindable<qreal> bindableRefreshRate() { return &this->bRefreshRate; }
	[[nodiscard]] QBindable<bool> bindableVrrSupported() { return &this->bVrrSupported; }
	[[nodiscard]] QBindable<bool> bindableVrrEnabled() { return &this->bVrrEnabled; }
	[[nodiscard]] QBindable<QString> bindableTransform() { return &this->bTransform; }

signals:
	void nameChanged();
	void connectedChanged();
	void scaleChanged();
	void widthChanged();
	void heightChanged();
	void xChanged();
	void yChanged();
	void physicalWidthChanged();
	void physicalHeightChanged();
	void refreshRateChanged();
	void vrrSupportedChanged();
	void vrrEnabledChanged();
	void transformChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NiriOutput, QString, bName, &NiriOutput::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriOutput, bool, bConnected, &NiriOutput::connectedChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(NiriOutput, qreal, bScale, 1.0, &NiriOutput::scaleChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriOutput, qint32, bWidth, &NiriOutput::widthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriOutput, qint32, bHeight, &NiriOutput::heightChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriOutput, qint32, bX, &NiriOutput::xChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriOutput, qint32, bY, &NiriOutput::yChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriOutput, qint32, bPhysicalWidth, &NiriOutput::physicalWidthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriOutput, qint32, bPhysicalHeight, &NiriOutput::physicalHeightChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriOutput, qreal, bRefreshRate, &NiriOutput::refreshRateChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriOutput, bool, bVrrSupported, &NiriOutput::vrrSupportedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriOutput, bool, bVrrEnabled, &NiriOutput::vrrEnabledChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriOutput, QString, bTransform, &NiriOutput::transformChanged);
	// clang-format on
};

} // namespace qs::niri::ipc
