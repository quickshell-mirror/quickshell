#include "qml.hpp"

#include <qobject.h>
#include <qproperty.h>
#include <qstring.h>
#include <qtypes.h>
#include <qcontainerfwd.h>

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
	QObject::connect(instance, &I3IpcController::activeBindingModeChanged, this, &I3IpcQml::activeBindingModeChanged);
	QObject::connect(instance, &I3IpcController::focusedScrollerChanged, this, &I3IpcQml::focusedScrollerChanged);
	QObject::connect(instance, &I3IpcController::focusedWindowChanged, this, &I3IpcQml::focusedWindowChanged);
	QObject::connect(instance, &I3IpcController::numberOfTrailsChanged, this, &I3IpcQml::numberOfTrailsChanged);
	QObject::connect(instance, &I3IpcController::activeTrailChanged, this, &I3IpcQml::activeTrailChanged);
	QObject::connect(instance, &I3IpcController::activeTrailLengthChanged, this, &I3IpcQml::activeTrailLengthChanged);
	// clang-format on
}

void I3IpcQml::dispatch(const QString& request) { I3IpcController::instance()->dispatch(request); }
void I3IpcQml::refreshMonitors() { I3IpcController::instance()->refreshMonitors(); }
void I3IpcQml::refreshWorkspaces() { I3IpcController::instance()->refreshWorkspaces(); }
void I3IpcQml::refreshBindingModes() { I3IpcController::instance()->refreshBindingModes(); }
void I3IpcQml::refreshScroller() { I3IpcController::instance()->refreshScroller(); }
void I3IpcQml::refreshTrails() { I3IpcController::instance()->refreshTrails(); }
QString I3IpcQml::socketPath() { return I3IpcController::instance()->socketPath(); }
QString I3IpcQml::compositor() { return I3IpcController::instance()->compositor(); }
ObjectModel<I3Monitor>* I3IpcQml::monitors() { return I3IpcController::instance()->monitors(); }
ObjectModel<I3Workspace>* I3IpcQml::workspaces() {
	return I3IpcController::instance()->workspaces();
}
QVector<QString>* I3IpcQml::bindingModes() {
	return I3IpcController::instance()->bindingModes();
}

QBindable<I3Workspace*> I3IpcQml::bindableFocusedWorkspace() {
	return I3IpcController::instance()->bindableFocusedWorkspace();
}

QBindable<I3Monitor*> I3IpcQml::bindableFocusedMonitor() {
	return I3IpcController::instance()->bindableFocusedMonitor();
}

QBindable<QString> I3IpcQml::bindableActiveBindingMode() {
	return I3IpcController::instance()->bindableActiveBindingMode();
}

QBindable<I3Scroller*> I3IpcQml::bindableFocusedScroller() {
	return I3IpcController::instance()->bindableFocusedScroller();
}

QBindable<I3Window*> I3IpcQml::bindableFocusedWindow() {
	return I3IpcController::instance()->bindableFocusedWindow();
}

QBindable<qint32> I3IpcQml::bindableNumberOfTrails() {
	return I3IpcController::instance()->bindableNumberOfTrails();
}

QBindable<qint32> I3IpcQml::bindableActiveTrail() {
	return I3IpcController::instance()->bindableActiveTrail();
}

QBindable<qint32> I3IpcQml::bindableActiveTrailLength() {
	return I3IpcController::instance()->bindableActiveTrailLength();
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
