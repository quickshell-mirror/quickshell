#pragma once

#include <qjsonarray.h>
#include <qobject.h>
#include <qproperty.h>

#include "../../../core/doc.hpp"
#include "../../../core/qmlscreen.hpp"
#include "connection.hpp"

namespace qs::i3::ipc {

///! I3/Sway IPC integration
class I3IpcQml: public QObject {
	Q_OBJECT;
	// clang-format off
	/// Path to the I3 socket
	Q_PROPERTY(QString socketPath READ socketPath CONSTANT);

	Q_PROPERTY(qs::i3::ipc::I3Workspace* focusedWorkspace READ default NOTIFY focusedWorkspaceChanged BINDABLE bindableFocusedWorkspace);
	Q_PROPERTY(qs::i3::ipc::I3Monitor* focusedMonitor READ default NOTIFY focusedMonitorChanged BINDABLE bindableFocusedMonitor);
	/// All I3 monitors.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::i3::ipc::I3Monitor>*);
	Q_PROPERTY(UntypedObjectModel* monitors READ monitors CONSTANT);
	/// All I3 workspaces.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::i3::ipc::I3Workspace>*);
	Q_PROPERTY(UntypedObjectModel* workspaces READ workspaces CONSTANT);
	// clang-format on
	QML_NAMED_ELEMENT(I3);
	QML_SINGLETON;

public:
	explicit I3IpcQml();

	/// Execute an [I3/Sway command](https://i3wm.org/docs/userguide.html#list_of_commands)
	Q_INVOKABLE static void dispatch(const QString& request);

	/// Refresh monitor information.
	Q_INVOKABLE static void refreshMonitors();

	/// Refresh workspace information.
	Q_INVOKABLE static void refreshWorkspaces();

	/// Find an I3Workspace using its name, returns null if the workspace doesn't exist.
	Q_INVOKABLE static I3Workspace* findWorkspaceByName(const QString& name);

	/// Find an I3Monitor using its name, returns null if the monitor doesn't exist.
	Q_INVOKABLE static I3Monitor* findMonitorByName(const QString& name);

	/// Return the i3/Sway monitor associated with `screen`
	Q_INVOKABLE static I3Monitor* monitorFor(QuickshellScreenInfo* screen);

	/// The path to the I3 or Sway socket currently being used
	[[nodiscard]] static QString socketPath();

	/// All I3Monitors
	[[nodiscard]] static ObjectModel<I3Monitor>* monitors();

	/// All I3Workspaces
	[[nodiscard]] static ObjectModel<I3Workspace>* workspaces();

	/// The currently focused Workspace
	[[nodiscard]] static QBindable<I3Workspace*> bindableFocusedWorkspace();

	/// The currently focused Monitor
	[[nodiscard]] static QBindable<I3Monitor*> bindableFocusedMonitor();

signals:
	void rawEvent(I3IpcEvent* event);
	void connected();
	void focusedWorkspaceChanged();
	void focusedMonitorChanged();
};

} // namespace qs::i3::ipc
