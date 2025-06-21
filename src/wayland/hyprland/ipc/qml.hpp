#pragma once

#include <qbytearrayview.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

#include "../../../core/doc.hpp"
#include "../../../core/model.hpp"
#include "../../../core/qmlscreen.hpp"
#include "connection.hpp"
#include "monitor.hpp"

namespace qs::hyprland::ipc {

class HyprlandIpcQml: public QObject {
	Q_OBJECT;
	// clang-format off
	/// Path to the request socket (.socket.sock)
	Q_PROPERTY(QString requestSocketPath READ requestSocketPath CONSTANT);
	/// Path to the event socket (.socket2.sock)
	Q_PROPERTY(QString eventSocketPath READ eventSocketPath CONSTANT);
	/// The currently focused hyprland monitor. May be null.
	Q_PROPERTY(qs::hyprland::ipc::HyprlandMonitor* focusedMonitor READ default NOTIFY focusedMonitorChanged BINDABLE bindableFocusedMonitor);
	/// The currently focused hyprland workspace. May be null.
	Q_PROPERTY(qs::hyprland::ipc::HyprlandWorkspace* focusedWorkspace READ default NOTIFY focusedWorkspaceChanged BINDABLE bindableFocusedWorkspace);
	/// Currently active toplevel (might be null)
	Q_PROPERTY(qs::hyprland::ipc::HyprlandToplevel* activeToplevel READ default NOTIFY activeToplevelChanged BINDABLE bindableActiveToplevel);
	/// All hyprland monitors.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::hyprland::ipc::HyprlandMonitor>*);
	Q_PROPERTY(UntypedObjectModel* monitors READ monitors CONSTANT);
	/// All hyprland workspaces, sorted by id.
	///
	/// > [!NOTE] Named workspaces have a negative id, and will appear before unnamed workspaces.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::hyprland::ipc::HyprlandWorkspace>*);
	Q_PROPERTY(UntypedObjectModel* workspaces READ workspaces CONSTANT);
	/// All hyprland toplevels
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::hyprland::ipc::HyprlandToplevel>*);
	Q_PROPERTY(UntypedObjectModel* toplevels READ toplevels CONSTANT);
	// clang-format on
	QML_NAMED_ELEMENT(Hyprland);
	QML_SINGLETON;

public:
	explicit HyprlandIpcQml();

	/// Execute a hyprland [dispatcher](https://wiki.hyprland.org/Configuring/Dispatchers).
	Q_INVOKABLE static void dispatch(const QString& request);

	/// Get the HyprlandMonitor object that corrosponds to a quickshell screen.
	Q_INVOKABLE static HyprlandMonitor* monitorFor(QuickshellScreenInfo* screen);

	/// Refresh monitor information.
	///
	/// Many actions that will invalidate monitor state don't send events,
	/// so this function is available if required.
	Q_INVOKABLE static void refreshMonitors();

	/// Refresh workspace information.
	///
	/// Many actions that will invalidate workspace state don't send events,
	/// so this function is available if required.
	Q_INVOKABLE static void refreshWorkspaces();

	/// Refresh toplevel information.
	///
	/// Many actions that will invalidate workspace state don't send events,
	/// so this function is available if required.
	Q_INVOKABLE static void refreshToplevels();

	[[nodiscard]] static QString requestSocketPath();
	[[nodiscard]] static QString eventSocketPath();
	[[nodiscard]] static QBindable<HyprlandMonitor*> bindableFocusedMonitor();
	[[nodiscard]] static QBindable<HyprlandWorkspace*> bindableFocusedWorkspace();
	[[nodiscard]] static QBindable<HyprlandToplevel*> bindableActiveToplevel();
	[[nodiscard]] static ObjectModel<HyprlandMonitor>* monitors();
	[[nodiscard]] static ObjectModel<HyprlandWorkspace>* workspaces();
	[[nodiscard]] static ObjectModel<HyprlandToplevel>* toplevels();

signals:
	/// Emitted for every event that comes in through the hyprland event socket (socket2).
	///
	/// See [Hyprland Wiki: IPC](https://wiki.hyprland.org/IPC/) for a list of events.
	void rawEvent(qs::hyprland::ipc::HyprlandIpcEvent* event);

	void focusedMonitorChanged();
	void focusedWorkspaceChanged();
	void activeToplevelChanged();
};

} // namespace qs::hyprland::ipc
