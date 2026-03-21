#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../../core/doc.hpp"
#include "../../../core/model.hpp"
#include "connection.hpp"
#include "output.hpp"
#include "window.hpp"
#include "workspace.hpp"

namespace qs::niri::ipc {

class NiriIpcQml: public QObject {
	Q_OBJECT;
	// clang-format off
	/// Path to the niri IPC socket.
	Q_PROPERTY(QString socketPath READ socketPath CONSTANT);
	/// The currently focused niri window. May be null.
	Q_PROPERTY(qs::niri::ipc::NiriWindow* focusedWindow READ default NOTIFY focusedWindowChanged BINDABLE bindableFocusedWindow);
	/// Whether the niri overview is currently open.
	Q_PROPERTY(bool overviewActive READ default NOTIFY overviewActiveChanged BINDABLE bindableOverviewActive);
	/// List of keyboard layout names.
	Q_PROPERTY(QStringList keyboardLayoutNames READ default NOTIFY keyboardLayoutsChanged BINDABLE bindableKeyboardLayoutNames);
	/// Index of the currently active keyboard layout.
	Q_PROPERTY(qint32 currentKeyboardLayoutIndex READ default NOTIFY keyboardLayoutSwitched BINDABLE bindableCurrentKeyboardLayoutIndex);
	/// The name of the currently active keyboard layout.
	Q_PROPERTY(QString currentKeyboardLayoutName READ currentKeyboardLayoutName NOTIFY keyboardLayoutSwitched);
	/// All niri outputs.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::niri::ipc::NiriOutput>*);
	Q_PROPERTY(UntypedObjectModel* outputs READ outputs CONSTANT);
	/// All niri workspaces, sorted by output then index.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::niri::ipc::NiriWorkspace>*);
	Q_PROPERTY(UntypedObjectModel* workspaces READ workspaces CONSTANT);
	/// All niri windows.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::niri::ipc::NiriWindow>*);
	Q_PROPERTY(UntypedObjectModel* windows READ windows CONSTANT);
	// clang-format on
	QML_NAMED_ELEMENT(Niri);
	QML_SINGLETON;

public:
	explicit NiriIpcQml();

	/// Execute a niri action via `niri msg action`.
	///
	/// Example: `Niri.dispatch(["focus-workspace", "1"])`
	Q_INVOKABLE static void dispatch(const QStringList& args);

	/// Refresh output information from niri.
	Q_INVOKABLE static void refreshOutputs();

	/// Refresh workspace information from niri.
	Q_INVOKABLE static void refreshWorkspaces();

	/// Refresh window information from niri.
	Q_INVOKABLE static void refreshWindows();

	[[nodiscard]] static QString socketPath();
	[[nodiscard]] static QBindable<NiriWindow*> bindableFocusedWindow();
	[[nodiscard]] static QBindable<bool> bindableOverviewActive();
	[[nodiscard]] static QBindable<QStringList> bindableKeyboardLayoutNames();
	[[nodiscard]] static QBindable<qint32> bindableCurrentKeyboardLayoutIndex();
	[[nodiscard]] static QString currentKeyboardLayoutName();
	[[nodiscard]] static ObjectModel<NiriOutput>* outputs();
	[[nodiscard]] static ObjectModel<NiriWorkspace>* workspaces();
	[[nodiscard]] static ObjectModel<NiriWindow>* windows();

signals:
	/// Emitted for every event received on the niri event stream.
	void rawEvent(qs::niri::ipc::NiriIpcEvent* event);

	void focusedWindowChanged();
	void overviewActiveChanged();
	void keyboardLayoutsChanged();
	void keyboardLayoutSwitched();

	/// Emitted whenever workspace data changes (items added/removed or properties updated).
	void workspacesUpdated();
	/// Emitted whenever window data changes (items added/removed or properties updated).
	void windowsUpdated();
	/// Emitted whenever output data changes (items added/removed or properties updated).
	void outputsUpdated();
};

} // namespace qs::niri::ipc
