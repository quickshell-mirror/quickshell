#include "qml.hpp"

#include <qobject.h>
#include <qproperty.h>
#include <qstring.h>

#include "../../../core/model.hpp"
#include "../../../core/qmlscreen.hpp"
#include "connection.hpp"
#include "controller.hpp"
#include "workspace.hpp"

namespace qs::i3::ipc {

I3IpcQml::I3IpcQml() {
	auto* instance = I3IpcController::instance();

	// clang-format off
	QObject::connect(instance, &I3Ipc::rawEvent, this, &I3IpcQml::rawEvent);
	QObject::connect(instance, &I3Ipc::connected, this, &I3IpcQml::connected);
	QObject::connect(instance, &I3IpcController::focusedWorkspaceChanged, this, &I3IpcQml::focusedWorkspaceChanged);
	QObject::connect(instance, &I3IpcController::focusedMonitorChanged, this, &I3IpcQml::focusedMonitorChanged);
	// clang-format on
}

void I3IpcQml::dispatch(const QString& request) { I3IpcController::instance()->dispatch(request); }
void I3IpcQml::refreshMonitors() { I3IpcController::instance()->refreshMonitors(); }
void I3IpcQml::refreshWorkspaces() { I3IpcController::instance()->refreshWorkspaces(); }
QString I3IpcQml::socketPath() { return I3IpcController::instance()->socketPath(); }
ObjectModel<I3Monitor>* I3IpcQml::monitors() { return I3IpcController::instance()->monitors(); }
ObjectModel<I3Workspace>* I3IpcQml::workspaces() {
	return I3IpcController::instance()->workspaces();
}

QBindable<I3Workspace*> I3IpcQml::bindableFocusedWorkspace() {
	return I3IpcController::instance()->bindableFocusedWorkspace();
}

QBindable<I3Monitor*> I3IpcQml::bindableFocusedMonitor() {
	return I3IpcController::instance()->bindableFocusedMonitor();
}

I3Workspace* I3IpcQml::findWorkspaceByName(const QString& name) {
	return I3IpcController::instance()->findWorkspaceByName(name);
}

I3Monitor* I3IpcQml::findMonitorByName(const QString& name) {
	return I3IpcController::instance()->findMonitorByName(name);
}

I3Monitor* I3IpcQml::monitorFor(QuickshellScreenInfo* screen) {
	return I3IpcController::instance()->monitorFor(screen);
}

} // namespace qs::i3::ipc
