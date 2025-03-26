#pragma once

#include <qbytearrayview.h>
#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "connection.hpp"

namespace qs::hyprland::ipc {

class HyprlandWorkspace;

class HyprlandMonitor: public QObject {
	Q_OBJECT;
	// clang-format off
	Q_PROPERTY(qint32 id READ default NOTIFY idChanged BINDABLE bindableId);
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);
	Q_PROPERTY(QString description READ default NOTIFY descriptionChanged BINDABLE bindableDescription);
	Q_PROPERTY(qint32 x READ default NOTIFY xChanged BINDABLE bindableX);
	Q_PROPERTY(qint32 y READ default NOTIFY yChanged BINDABLE bindableY);
	Q_PROPERTY(qint32 width READ default NOTIFY widthChanged BINDABLE bindableWidth);
	Q_PROPERTY(qint32 height READ default NOTIFY heightChanged BINDABLE bindableHeight);
	Q_PROPERTY(qreal scale READ default NOTIFY scaleChanged BINDABLE bindableScale);
	/// Last json returned for this monitor, as a javascript object.
	///
	/// > [!WARNING] This is *not* updated unless the monitor object is fetched again from
	/// > Hyprland. If you need a value that is subject to change and does not have a dedicated
	/// > property, run @@Hyprland.refreshMonitors() and wait for this property to update.
	Q_PROPERTY(QVariantMap lastIpcObject READ lastIpcObject NOTIFY lastIpcObjectChanged);
	/// The currently active workspace on this monitor. May be null.
	Q_PROPERTY(qs::hyprland::ipc::HyprlandWorkspace* activeWorkspace READ default NOTIFY activeWorkspaceChanged BINDABLE bindableActiveWorkspace);
	/// If the monitor is currently focused.
	Q_PROPERTY(bool focused READ default NOTIFY focusedChanged BINDABLE bindableFocused);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("HyprlandMonitors must be retrieved from the HyprlandIpc object.");

public:
	explicit HyprlandMonitor(HyprlandIpc* ipc): QObject(ipc), ipc(ipc) {}

	void updateInitial(qint32 id, const QString& name, const QString& description);
	void updateFromObject(QVariantMap object);

	[[nodiscard]] QBindable<qint32> bindableId() { return &this->bId; }
	[[nodiscard]] QBindable<QString> bindableName() { return &this->bName; }
	[[nodiscard]] QBindable<QString> bindableDescription() { return &this->bDescription; }
	[[nodiscard]] QBindable<qint32> bindableX() { return &this->bX; }
	[[nodiscard]] QBindable<qint32> bindableY() { return &this->bY; }
	[[nodiscard]] QBindable<qint32> bindableWidth() { return &this->bWidth; }
	[[nodiscard]] QBindable<qint32> bindableHeight() { return &this->bHeight; }
	[[nodiscard]] QBindable<qreal> bindableScale() { return &this->bScale; }

	[[nodiscard]] QBindable<HyprlandWorkspace*> bindableActiveWorkspace() const {
		return &this->bActiveWorkspace;
	}

	[[nodiscard]] QBindable<bool> bindableFocused() const { return &this->bFocused; }

	[[nodiscard]] QVariantMap lastIpcObject() const;

	void setActiveWorkspace(HyprlandWorkspace* workspace);

signals:
	void idChanged();
	void nameChanged();
	void descriptionChanged();
	void xChanged();
	void yChanged();
	void widthChanged();
	void heightChanged();
	void scaleChanged();
	void lastIpcObjectChanged();
	void activeWorkspaceChanged();
	void focusedChanged();

private slots:
	void onActiveWorkspaceDestroyed();

private:
	HyprlandIpc* ipc;

	QVariantMap mLastIpcObject;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HyprlandMonitor, qint32, bId, -1, &HyprlandMonitor::idChanged);
	Q_OBJECT_BINDABLE_PROPERTY(HyprlandMonitor, QString, bName, &HyprlandMonitor::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(HyprlandMonitor, QString, bDescription, &HyprlandMonitor::descriptionChanged);
	Q_OBJECT_BINDABLE_PROPERTY(HyprlandMonitor, qint32, bX, &HyprlandMonitor::xChanged);
	Q_OBJECT_BINDABLE_PROPERTY(HyprlandMonitor, qint32, bY, &HyprlandMonitor::yChanged);
	Q_OBJECT_BINDABLE_PROPERTY(HyprlandMonitor, qint32, bWidth, &HyprlandMonitor::widthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(HyprlandMonitor, qint32, bHeight, &HyprlandMonitor::heightChanged);
	Q_OBJECT_BINDABLE_PROPERTY(HyprlandMonitor, qreal, bScale, &HyprlandMonitor::scaleChanged);
	Q_OBJECT_BINDABLE_PROPERTY(HyprlandMonitor, HyprlandWorkspace*, bActiveWorkspace, &HyprlandMonitor::activeWorkspaceChanged);
	Q_OBJECT_BINDABLE_PROPERTY(HyprlandMonitor, bool, bFocused, &HyprlandMonitor::focusedChanged);
	// clang-format on
};

} // namespace qs::hyprland::ipc
