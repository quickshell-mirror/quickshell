#pragma once

#include <qjsonarray.h>
#include <qobject.h>
#include <qproperty.h>

#include "../../../core/doc.hpp"
#include "../../../core/qmlscreen.hpp"
#include "connection.hpp"
#include "controller.hpp"

namespace qs::i3::ipc {

///! I3/Sway IPC integration
class I3IpcQml: public QObject {
	Q_OBJECT;
	// clang-format off
	/// Path to the I3 socket
	Q_PROPERTY(QString socketPath READ socketPath CONSTANT);

	Q_PROPERTY(qs::i3::ipc::I3Workspace* focusedWorkspace READ default NOTIFY focusedWorkspaceChanged BINDABLE bindableFocusedWorkspace);
	Q_PROPERTY(qs::i3::ipc::I3Monitor* focusedMonitor READ default NOTIFY focusedMonitorChanged BINDABLE bindableFocusedMonitor);
	Q_PROPERTY(QString activeBindingMode READ default NOTIFY activeBindingModeChanged BINDABLE bindableActiveBindingMode);
	Q_PROPERTY(qs::i3::ipc::I3Scroller* focusedScroller READ default NOTIFY focusedScrollerChanged BINDABLE bindableFocusedScroller);
	Q_PROPERTY(qs::i3::ipc::I3Window* focusedWindow READ default NOTIFY focusedWindowChanged BINDABLE bindableFocusedWindow);
	Q_PROPERTY(qint32 numberOfTrails READ default NOTIFY numberOfTrailsChanged BINDABLE bindableNumberOfTrails);
	Q_PROPERTY(qint32 activeTrail READ default NOTIFY activeTrailChanged BINDABLE bindableActiveTrail);
	Q_PROPERTY(qint32 activeTrailLength READ default NOTIFY activeTrailLengthChanged BINDABLE bindableActiveTrailLength);
	Q_PROPERTY(QByteArray luaData READ default NOTIFY luaDataChanged BINDABLE bindableLuaData);
	/// All I3 monitors.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::i3::ipc::I3Monitor>*);
	Q_PROPERTY(UntypedObjectModel* monitors READ monitors CONSTANT);
	/// All I3 workspaces.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::i3::ipc::I3Workspace>*);
	Q_PROPERTY(UntypedObjectModel* workspaces READ workspaces CONSTANT);
	/// All I3 binding modes.
	Q_PROPERTY(QVector<QString>* bindingModes READ bindingModes CONSTANT);
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

	/// Refresh binding modes information.
	Q_INVOKABLE static void refreshBindingModes();

	/// Refresh scroller information.
	Q_INVOKABLE static void refreshScroller();

	/// Refresh trails information.
	Q_INVOKABLE static void refreshTrails();

	/// Find an I3Workspace using its name, returns null if the workspace doesn't exist.
	Q_INVOKABLE static I3Workspace* findWorkspaceByName(const QString& name);

	/// Find an I3Monitor using its name, returns null if the monitor doesn't exist.
	Q_INVOKABLE static I3Monitor* findMonitorByName(const QString& name);

	/// Return the i3/Sway monitor associated with `screen`
	Q_INVOKABLE static I3Monitor* monitorFor(QuickshellScreenInfo* screen);

	/// The path to the I3 or Sway socket currently being used
	[[nodiscard]] static QString socketPath();

	/// The compositor providing the IPC socket: i3, sway or scroll
	[[nodiscard]] static QString compositor();

	/// All I3Monitors
	[[nodiscard]] static ObjectModel<I3Monitor>* monitors();

	/// All I3Workspaces
	[[nodiscard]] static ObjectModel<I3Workspace>* workspaces();

	/// All I3 binding modes
	[[nodiscard]] static QVector<QString>* bindingModes();

	/// The currently focused Workspace
	[[nodiscard]] static QBindable<I3Workspace*> bindableFocusedWorkspace();

	/// The currently focused Monitor
	[[nodiscard]] static QBindable<I3Monitor*> bindableFocusedMonitor();

	/// The currently focused Monitor
	[[nodiscard]] static QBindable<QString> bindableActiveBindingMode();

	/// The currently focused Scroller
	[[nodiscard]] static QBindable<I3Scroller*> bindableFocusedScroller();

	/// The currently focused Window
	[[nodiscard]] static QBindable<I3Window*> bindableFocusedWindow();

	/// The current number of trails
	[[nodiscard]] static QBindable<qint32> bindableNumberOfTrails();

	/// The current active trail index
	[[nodiscard]] static QBindable<qint32> bindableActiveTrail();

	/// The length of the current active trail
	[[nodiscard]] static QBindable<qint32> bindableActiveTrailLength();

	/// The current JSON document received from Lua scripts
	[[nodiscard]] static QBindable<QByteArray> bindableLuaData();

signals:
	void rawEvent(I3IpcEvent* event);
	void connected();
	void focusedWorkspaceChanged();
	void focusedMonitorChanged();
	void activeBindingModeChanged();
	void focusedScrollerChanged();
	void focusedWindowChanged();
	void numberOfTrailsChanged();
	void activeTrailChanged();
	void activeTrailLengthChanged();
	void luaDataChanged();
};

} // namespace qs::i3::ipc
