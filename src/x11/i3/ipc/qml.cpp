#include "qml.hpp"

#include <qobject.h>
#include <qproperty.h>
#include <qstring.h>

#include "../../../core/model.hpp"
#include "../../../core/qmlscreen.hpp"
#include "connection.hpp"
#include "workspace.hpp"

namespace qs::i3::ipc {

I3IpcQml::I3IpcQml() {
	auto* instance = I3Ipc::instance();

	// clang-format off
	QObject::connect(instance, &I3Ipc::rawEvent, this, &I3IpcQml::rawEvent);
	QObject::connect(instance, &I3Ipc::connected, this, &I3IpcQml::connected);
	QObject::connect(instance, &I3Ipc::focusedWorkspaceChanged, this, &I3IpcQml::focusedWorkspaceChanged);
	QObject::connect(instance, &I3Ipc::focusedMonitorChanged, this, &I3IpcQml::focusedMonitorChanged);
	// clang-format on
}

void I3IpcQml::dispatch(const QString& request) { I3Ipc::instance()->dispatch(request); }
void I3IpcQml::refreshMonitors() { I3Ipc::instance()->refreshMonitors(); }
void I3IpcQml::refreshWorkspaces() { I3Ipc::instance()->refreshWorkspaces(); }
QString I3IpcQml::socketPath() { return I3Ipc::instance()->socketPath(); }
ObjectModel<I3Monitor>* I3IpcQml::monitors() { return I3Ipc::instance()->monitors(); }
ObjectModel<I3Workspace>* I3IpcQml::workspaces() { return I3Ipc::instance()->workspaces(); }

QBindable<I3Workspace*> I3IpcQml::bindableFocusedWorkspace() {
	return I3Ipc::instance()->bindableFocusedWorkspace();
}

QBindable<I3Monitor*> I3IpcQml::bindableFocusedMonitor() {
	return I3Ipc::instance()->bindableFocusedMonitor();
}

I3Workspace* I3IpcQml::findWorkspaceByName(const QString& name) {
	return I3Ipc::instance()->findWorkspaceByName(name);
}

I3Monitor* I3IpcQml::findMonitorByName(const QString& name) {
	return I3Ipc::instance()->findMonitorByName(name);
}

I3Monitor* I3IpcQml::monitorFor(QuickshellScreenInfo* screen) {
	return I3Ipc::instance()->monitorFor(screen);
}

} // namespace qs::i3::ipc
