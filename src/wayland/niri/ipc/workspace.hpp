#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "connection.hpp"

namespace qs::niri::ipc {

class NiriMonitor;

class NiriWorkspace: public QObject {
	Q_OBJECT;
	// clang-format off
	/// Niri's unique workspace id. Remains constant while the workspace exists.
	Q_PROPERTY(qint64 id READ default NOTIFY idChanged BINDABLE bindableId);
	/// Index of the workspace on its monitor. Changes as workspaces are moved.
	Q_PROPERTY(qint32 idx READ default NOTIFY idxChanged BINDABLE bindableIdx);
	/// Name of the workspace. Empty for unnamed workspaces.
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);
	/// If this workspace is currently active on its monitor. See also @@focused.
	Q_PROPERTY(bool active READ default NOTIFY activeChanged BINDABLE bindableActive);
	/// If this workspace is currently focused. Only one workspace is focused across all outputs.
	Q_PROPERTY(bool focused READ default NOTIFY focusedChanged BINDABLE bindableFocused);
	/// If this workspace contains an urgent window.
	Q_PROPERTY(bool urgent READ default NOTIFY urgentChanged BINDABLE bindableUrgent);
	/// The monitor this workspace is on. May be null if no outputs are connected.
	Q_PROPERTY(qs::niri::ipc::NiriMonitor* monitor READ default NOTIFY monitorChanged BINDABLE bindableMonitor);
	/// Id of the active window on this workspace, or 0 if none.
	Q_PROPERTY(qint64 activeWindowId READ default NOTIFY activeWindowIdChanged BINDABLE bindableActiveWindowId);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("NiriWorkspaces must be retrieved from the Niri singleton.");

public:
	explicit NiriWorkspace(NiriIpc* ipc);

	void updateFromObject(const QVariantMap& object);

	/// Focus this workspace.
	Q_INVOKABLE void activate();

	[[nodiscard]] QBindable<qint64> bindableId() { return &this->bId; }
	[[nodiscard]] QBindable<qint32> bindableIdx() { return &this->bIdx; }
	[[nodiscard]] QBindable<QString> bindableName() { return &this->bName; }
	[[nodiscard]] QBindable<bool> bindableActive() { return &this->bActive; }
	[[nodiscard]] QBindable<bool> bindableFocused() { return &this->bFocused; }
	[[nodiscard]] QBindable<bool> bindableUrgent() { return &this->bUrgent; }
	[[nodiscard]] QBindable<NiriMonitor*> bindableMonitor() { return &this->bMonitor; }
	[[nodiscard]] QBindable<qint64> bindableActiveWindowId() { return &this->bActiveWindowId; }

	void setMonitor(NiriMonitor* monitor);

signals:
	void idChanged();
	void idxChanged();
	void nameChanged();
	void activeChanged();
	void focusedChanged();
	void urgentChanged();
	void monitorChanged();
	void activeWindowIdChanged();

private slots:
	void onMonitorDestroyed();

private:
	NiriIpc* ipc;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(NiriWorkspace, qint64, bId, 0, &NiriWorkspace::idChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWorkspace, qint32, bIdx, &NiriWorkspace::idxChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWorkspace, QString, bName, &NiriWorkspace::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWorkspace, bool, bActive, &NiriWorkspace::activeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWorkspace, bool, bFocused, &NiriWorkspace::focusedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWorkspace, bool, bUrgent, &NiriWorkspace::urgentChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWorkspace, NiriMonitor*, bMonitor, &NiriWorkspace::monitorChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWorkspace, qint64, bActiveWindowId, &NiriWorkspace::activeWindowIdChanged);
	// clang-format on
};

} // namespace qs::niri::ipc
