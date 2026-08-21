#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

#include "connection.hpp"
#include "workspace.hpp"

namespace qs::niri::ipc {

class NiriMonitor: public QObject {
	Q_OBJECT;
	// clang-format off
	/// Name of the output (e.g. "DP-1").
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);
	/// Textual description of the manufacturer.
	Q_PROPERTY(QString make READ default NOTIFY makeChanged BINDABLE bindableMake);
	/// Textual description of the model.
	Q_PROPERTY(QString model READ default NOTIFY modelChanged BINDABLE bindableModel);
	/// Logical position of the output. Zero when the output is disabled.
	Q_PROPERTY(qint32 x READ default NOTIFY xChanged BINDABLE bindableX);
	/// Logical position of the output. Zero when the output is disabled.
	Q_PROPERTY(qint32 y READ default NOTIFY yChanged BINDABLE bindableY);
	/// Logical width of the output. Zero when the output is disabled.
	Q_PROPERTY(qint32 width READ default NOTIFY widthChanged BINDABLE bindableWidth);
	/// Logical height of the output. Zero when the output is disabled.
	Q_PROPERTY(qint32 height READ default NOTIFY heightChanged BINDABLE bindableHeight);
	/// Scale factor of the output. Zero when the output is disabled.
	Q_PROPERTY(qreal scale READ default NOTIFY scaleChanged BINDABLE bindableScale);
	/// Last json returned for this monitor, as a javascript object.
	///
	/// > [!WARNING] This is *not* updated automatically. Niri sends no output events,
	/// > run @@Niri.refreshMonitors() and wait for this property to update.
	Q_PROPERTY(QVariantMap lastIpcObject READ lastIpcObject NOTIFY lastIpcObjectChanged);
	/// The currently active workspace on this monitor. May be null.
	Q_PROPERTY(qs::niri::ipc::NiriWorkspace* activeWorkspace READ default NOTIFY activeWorkspaceChanged BINDABLE bindableActiveWorkspace);
	/// If the monitor is currently focused.
	Q_PROPERTY(bool focused READ default NOTIFY focusedChanged BINDABLE bindableFocused);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("NiriMonitors must be retrieved from the Niri singleton.");

public:
	explicit NiriMonitor(NiriIpc* ipc);

	void updateInitial(const QString& name);
	void updateFromObject(const QVariantMap& object);

	[[nodiscard]] QBindable<QString> bindableName() { return &this->bName; }
	[[nodiscard]] QBindable<QString> bindableMake() { return &this->bMake; }
	[[nodiscard]] QBindable<QString> bindableModel() { return &this->bModel; }
	[[nodiscard]] QBindable<qint32> bindableX() { return &this->bX; }
	[[nodiscard]] QBindable<qint32> bindableY() { return &this->bY; }
	[[nodiscard]] QBindable<qint32> bindableWidth() { return &this->bWidth; }
	[[nodiscard]] QBindable<qint32> bindableHeight() { return &this->bHeight; }
	[[nodiscard]] QBindable<qreal> bindableScale() { return &this->bScale; }

	[[nodiscard]] QBindable<NiriWorkspace*> bindableActiveWorkspace() {
		return &this->bActiveWorkspace;
	}

	[[nodiscard]] QBindable<bool> bindableFocused() { return &this->bFocused; }

	[[nodiscard]] QVariantMap lastIpcObject() const;

	void setActiveWorkspace(NiriWorkspace* workspace);

signals:
	void nameChanged();
	void makeChanged();
	void modelChanged();
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
	NiriIpc* ipc;
	QVariantMap mLastIpcObject;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(NiriMonitor, QString, bName, &NiriMonitor::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriMonitor, QString, bMake, &NiriMonitor::makeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriMonitor, QString, bModel, &NiriMonitor::modelChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriMonitor, qint32, bX, &NiriMonitor::xChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriMonitor, qint32, bY, &NiriMonitor::yChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriMonitor, qint32, bWidth, &NiriMonitor::widthChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriMonitor, qint32, bHeight, &NiriMonitor::heightChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriMonitor, qreal, bScale, &NiriMonitor::scaleChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriMonitor, NiriWorkspace*, bActiveWorkspace, &NiriMonitor::activeWorkspaceChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriMonitor, bool, bFocused, &NiriMonitor::focusedChanged);
	// clang-format on
};

} // namespace qs::niri::ipc
