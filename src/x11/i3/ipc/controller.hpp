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
class I3Scroller;
class I3Window;
} // namespace qs::i3::ipc

Q_DECLARE_OPAQUE_POINTER(qs::i3::ipc::I3Workspace*);
Q_DECLARE_OPAQUE_POINTER(qs::i3::ipc::I3Monitor*);
Q_DECLARE_OPAQUE_POINTER(qs::i3::ipc::I3Scroller*);
Q_DECLARE_OPAQUE_POINTER(qs::i3::ipc::I3Window*);

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
	void refreshBindingModes();
	void refreshScroller();
	void refreshTrails();

	I3Monitor* monitorFor(QuickshellScreenInfo* screen);

	[[nodiscard]] QBindable<I3Monitor*> bindableFocusedMonitor() const {
		return &this->bFocusedMonitor;
	};

	[[nodiscard]] QBindable<I3Workspace*> bindableFocusedWorkspace() const {
		return &this->bFocusedWorkspace;
	};

	[[nodiscard]] QBindable<QString> bindableActiveBindingMode() const {
		return &this->bActiveBindingMode;
	};

	[[nodiscard]] QBindable<I3Scroller*> bindableFocusedScroller() const {
		return &this->bFocusedScroller;
	};

	[[nodiscard]] QBindable<I3Window*> bindableFocusedWindow() const {
		return &this->bFocusedWindow;
	};

	[[nodiscard]] QBindable<qint32> bindableNumberOfTrails() const {
		return &this->bNumberOfTrails;
	};

	[[nodiscard]] QBindable<qint32> bindableActiveTrail() const {
		return &this->bActiveTrail;
	};

	[[nodiscard]] QBindable<qint32> bindableActiveTrailLength() const {
		return &this->bActiveTrailLength;
	};

	[[nodiscard]] ObjectModel<I3Monitor>* monitors();
	[[nodiscard]] ObjectModel<I3Workspace>* workspaces();
	[[nodiscard]] QVector<QString>* bindingModes();

signals:
	void focusedWorkspaceChanged();
	void focusedMonitorChanged();
	void activeBindingModeChanged();
	void focusedScrollerChanged();
	void focusedWindowChanged();
	void numberOfTrailsChanged();
	void activeTrailChanged();
	void activeTrailLengthChanged();

private slots:
	void onFocusedMonitorDestroyed();

	void onEvent(I3IpcEvent* event);
	void onConnected();

private:
	explicit I3IpcController();

	void handleWorkspaceEvent(I3IpcEvent* event);
	void handleGetWorkspacesEvent(I3IpcEvent* event);
	void handleGetOutputsEvent(I3IpcEvent* event);
	void handleGetBindingModesEvent(I3IpcEvent* event);
	void handleGetBindingStateEvent(I3IpcEvent* event);
	void handleModeEvent(I3IpcEvent* event);
	void handleGetScrollerEvent(I3IpcEvent* event);
	void handleScrollerEvent(I3IpcEvent* event);
	void handleGetTrailsEvent(I3IpcEvent* event);
	void handleTrailsEvent(I3IpcEvent* event);
	void handleWindowEvent(I3IpcEvent* event);
	static void handleRunCommand(I3IpcEvent* event);
	static bool compareWorkspaces(I3Workspace* a, I3Workspace* b);

	ObjectModel<I3Monitor> mMonitors {this};
	ObjectModel<I3Workspace> mWorkspaces {this};

	QVector<QString> mBindingModes;

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

	Q_OBJECT_BINDABLE_PROPERTY(
	    I3IpcController,
	    QString,
	    bActiveBindingMode,
	    &I3IpcController::activeBindingModeChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY(
	    I3IpcController,
	    I3Scroller*,
	    bFocusedScroller,
	    &I3IpcController::focusedScrollerChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY(
	    I3IpcController,
	    I3Window*,
	    bFocusedWindow,
	    &I3IpcController::focusedWindowChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY(
	    I3IpcController,
	    qint32,
	    bNumberOfTrails,
	    &I3IpcController::numberOfTrailsChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY(
	    I3IpcController,
	    qint32,
	    bActiveTrail,
	    &I3IpcController::activeTrailChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY(
	    I3IpcController,
	    qint32,
	    bActiveTrailLength,
	    &I3IpcController::activeTrailLengthChanged
	);
};

} // namespace qs::i3::ipc
