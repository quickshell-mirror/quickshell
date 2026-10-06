#pragma once

#include <qbytearrayview.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qstringlist.h>
#include <qtmetamacros.h>

#include "../../../core/model.hpp"
#include "../../../core/qmlscreen.hpp"
#include "cast.hpp"
#include "connection.hpp"
#include "monitor.hpp"
#include "window.hpp"
#include "workspace.hpp"

namespace qs::niri::ipc {

///! Niri IPC integration.
/// Exposes live niri compositor state: workspaces, windows, monitors,
/// screencasts, the overview, and keyboard layouts.
///
/// > [!NOTE] Unlike the Hyprland module, niri windows cannot be associated with
/// > @@Quickshell.Wayland.Toplevel handles, as niri does not expose its window
/// > ids over wayland. @@NiriWindow objects are purely informational.
class NiriIpcQml: public QObject {
	Q_OBJECT;
	// clang-format off
	/// Path to the niri IPC socket ($NIRI_SOCKET).
	Q_PROPERTY(QString socketPath READ socketPath CONSTANT);
	/// The currently focused niri monitor. May be null.
	Q_PROPERTY(qs::niri::ipc::NiriMonitor* focusedMonitor READ default NOTIFY focusedMonitorChanged BINDABLE bindableFocusedMonitor);
	/// The currently focused niri workspace. May be null.
	Q_PROPERTY(qs::niri::ipc::NiriWorkspace* focusedWorkspace READ default NOTIFY focusedWorkspaceChanged BINDABLE bindableFocusedWorkspace);
	/// Currently active window. May be null.
	Q_PROPERTY(qs::niri::ipc::NiriWindow* activeWindow READ default NOTIFY activeWindowChanged BINDABLE bindableActiveWindow);
	/// If the niri overview is currently open.
	Q_PROPERTY(bool overviewOpen READ default NOTIFY overviewOpenChanged BINDABLE bindableOverviewOpen);
	/// XKB names of the configured keyboard layouts.
	Q_PROPERTY(QStringList keyboardLayouts READ keyboardLayouts NOTIFY keyboardLayoutsChanged);
	/// Index of the currently active layout in @@keyboardLayouts.
	Q_PROPERTY(qint32 currentKeyboardLayout READ default NOTIFY currentKeyboardLayoutChanged BINDABLE bindableCurrentKeyboardLayout);
	/// All niri monitors.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::niri::ipc::NiriMonitor>*);
	Q_PROPERTY(UntypedObjectModel* monitors READ monitors CONSTANT);
	/// All niri workspaces, sorted by monitor and index.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::niri::ipc::NiriWorkspace>*);
	Q_PROPERTY(UntypedObjectModel* workspaces READ workspaces CONSTANT);
	/// All niri windows.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::niri::ipc::NiriWindow>*);
	Q_PROPERTY(UntypedObjectModel* windows READ windows CONSTANT);
	/// All niri screencasts.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::niri::ipc::NiriCast>*);
	Q_PROPERTY(UntypedObjectModel* casts READ casts CONSTANT);
	// clang-format on
	QML_NAMED_ELEMENT(Niri);
	QML_SINGLETON;

public:
	explicit NiriIpcQml();

	/// Execute a niri action, given as a JSON string.
	///
	/// ```qml
	/// Niri.dispatch('{"FocusWorkspaceDown":{}}');
	/// ```
	///
	/// See the [niri-ipc documentation](https://niri-wm.github.io/niri/niri_ipc/enum.Action.html)
	/// for available actions.
	Q_INVOKABLE static void dispatch(const QString& request);

	/// Get the NiriMonitor object that corresponds to a quickshell screen.
	Q_INVOKABLE static NiriMonitor* monitorFor(QuickshellScreenInfo* screen);

	/// Refresh monitor information.
	///
	/// Niri sends no events for output changes, so this function must be
	/// called to update monitor state (e.g. after hotplugging a display).
	Q_INVOKABLE static void refreshMonitors();

	/// Refresh workspace information.
	///
	/// The event stream keeps workspaces up to date, so this is usually
	/// not required.
	Q_INVOKABLE static void refreshWorkspaces();

	/// Refresh window information.
	///
	/// The event stream keeps windows up to date, so this is usually
	/// not required.
	Q_INVOKABLE static void refreshWindows();

	/// Refresh screencast information.
	///
	/// The event stream keeps casts up to date, so this is usually
	/// not required.
	Q_INVOKABLE static void refreshCasts();

	[[nodiscard]] static QString socketPath();
	[[nodiscard]] static QBindable<NiriMonitor*> bindableFocusedMonitor();
	[[nodiscard]] static QBindable<NiriWorkspace*> bindableFocusedWorkspace();
	[[nodiscard]] static QBindable<NiriWindow*> bindableActiveWindow();
	[[nodiscard]] static QBindable<bool> bindableOverviewOpen();
	[[nodiscard]] static QBindable<qint32> bindableCurrentKeyboardLayout();
	[[nodiscard]] static QStringList keyboardLayouts();
	[[nodiscard]] static ObjectModel<NiriMonitor>* monitors();
	[[nodiscard]] static ObjectModel<NiriWorkspace>* workspaces();
	[[nodiscard]] static ObjectModel<NiriWindow>* windows();
	[[nodiscard]] static ObjectModel<NiriCast>* casts();

signals:
	/// Emitted for every event that comes in through the niri event stream.
	///
	/// See the [niri-ipc documentation](https://niri-wm.github.io/niri/niri_ipc/enum.Event.html)
	/// for a list of events.
	void rawEvent(qs::niri::ipc::NiriIpcEvent* event);

	/// Emitted when niri reloads its configuration. Also emitted once when
	/// quickshell connects to niri, indicating the last config load attempt.
	void configLoaded(bool failed);
	/// Emitted when niri captures a screenshot. Empty path when the screenshot
	/// was only copied to the clipboard.
	void screenshotCaptured(const QString& path);

	void focusedMonitorChanged();
	void focusedWorkspaceChanged();
	void activeWindowChanged();
	void overviewOpenChanged();
	void keyboardLayoutsChanged();
	void currentKeyboardLayoutChanged();
};

} // namespace qs::niri::ipc
