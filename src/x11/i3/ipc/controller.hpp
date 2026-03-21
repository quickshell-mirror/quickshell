#pragma once

#include <qbytearrayview.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqml.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../../core/model.hpp"
#include "../../../core/qmlscreen.hpp"
#include "connection.hpp"

namespace qs::i3::ipc {

class I3Workspace;
class I3Monitor;
} // namespace qs::i3::ipc

Q_DECLARE_OPAQUE_POINTER(qs::i3::ipc::I3Workspace*);
Q_DECLARE_OPAQUE_POINTER(qs::i3::ipc::I3Monitor*);

namespace qs::i3::ipc {

/// I3/Sway IPC controller that manages workspaces and monitors
class I3IpcController: public I3Ipc {
	Q_OBJECT;

public:
	static I3IpcController* instance();

	I3Workspace* findWorkspaceByName(const QString& name);
	I3Monitor* findMonitorByName(const QString& name, bool createIfMissing = false);
	I3Workspace* findWorkspaceByID(qint32 id);

	void setFocusedMonitor(I3Monitor* monitor);

	void refreshWorkspaces();
	void refreshMonitors();

	I3Monitor* monitorFor(QuickshellScreenInfo* screen);

	[[nodiscard]] QBindable<I3Monitor*> bindableFocusedMonitor() const {
		return &this->bFocusedMonitor;
	};

	[[nodiscard]] QBindable<I3Workspace*> bindableFocusedWorkspace() const {
		return &this->bFocusedWorkspace;
	};

	[[nodiscard]] ObjectModel<I3Monitor>* monitors();
	[[nodiscard]] ObjectModel<I3Workspace>* workspaces();

signals:
	void focusedWorkspaceChanged();
	void focusedMonitorChanged();

private slots:
	void onFocusedMonitorDestroyed();

	void onEvent(I3IpcEvent* event);
	void onConnected();

private:
	explicit I3IpcController();

	void handleWorkspaceEvent(I3IpcEvent* event);
	void handleGetWorkspacesEvent(I3IpcEvent* event);
	void handleGetOutputsEvent(I3IpcEvent* event);
	static void handleRunCommand(I3IpcEvent* event);
	static bool compareWorkspaces(I3Workspace* a, I3Workspace* b);

	ObjectModel<I3Monitor> mMonitors {this};
	ObjectModel<I3Workspace> mWorkspaces {this};

	Q_OBJECT_BINDABLE_PROPERTY(
	    I3IpcController,
	    I3Monitor*,
	    bFocusedMonitor,
	    &I3IpcController::focusedMonitorChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY(
	    I3IpcController,
	    I3Workspace*,
	    bFocusedWorkspace,
	    &I3IpcController::focusedWorkspaceChanged
	);
};

} // namespace qs::i3::ipc
