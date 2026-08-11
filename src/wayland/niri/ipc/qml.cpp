#include "qml.hpp"

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qstringlist.h>
#include <qtypes.h>

#include "../../../core/model.hpp"
#include "../../../core/qmlscreen.hpp"
#include "cast.hpp"
#include "connection.hpp"
#include "monitor.hpp"
#include "window.hpp"
#include "workspace.hpp"

namespace qs::niri::ipc {

NiriIpcQml::NiriIpcQml() {
	auto* instance = NiriIpc::instance();

	QObject::connect(instance, &NiriIpc::rawEvent, this, &NiriIpcQml::rawEvent);

	QObject::connect(
	    instance,
	    &NiriIpc::focusedMonitorChanged,
	    this,
	    &NiriIpcQml::focusedMonitorChanged
	);

	QObject::connect(
	    instance,
	    &NiriIpc::focusedWorkspaceChanged,
	    this,
	    &NiriIpcQml::focusedWorkspaceChanged
	);

	QObject::connect(instance, &NiriIpc::activeWindowChanged, this, &NiriIpcQml::activeWindowChanged);

	QObject::connect(instance, &NiriIpc::overviewOpenChanged, this, &NiriIpcQml::overviewOpenChanged);

	QObject::connect(
	    instance,
	    &NiriIpc::keyboardLayoutsChanged,
	    this,
	    &NiriIpcQml::keyboardLayoutsChanged
	);

	QObject::connect(
	    instance,
	    &NiriIpc::currentKeyboardLayoutChanged,
	    this,
	    &NiriIpcQml::currentKeyboardLayoutChanged
	);

	QObject::connect(instance, &NiriIpc::configLoaded, this, &NiriIpcQml::configLoaded);
	QObject::connect(instance, &NiriIpc::screenshotCaptured, this, &NiriIpcQml::screenshotCaptured);
}

void NiriIpcQml::dispatch(const QString& request) { NiriIpc::instance()->dispatch(request); }

NiriMonitor* NiriIpcQml::monitorFor(QuickshellScreenInfo* screen) {
	return NiriIpc::instance()->monitorFor(screen);
}

void NiriIpcQml::refreshMonitors() { NiriIpc::instance()->refreshMonitors(); }
void NiriIpcQml::refreshWorkspaces() { NiriIpc::instance()->refreshWorkspaces(); }
void NiriIpcQml::refreshWindows() { NiriIpc::instance()->refreshWindows(); }
void NiriIpcQml::refreshCasts() { NiriIpc::instance()->refreshCasts(); }
QString NiriIpcQml::socketPath() { return NiriIpc::instance()->socketPath(); }

QBindable<NiriMonitor*> NiriIpcQml::bindableFocusedMonitor() {
	return NiriIpc::instance()->bindableFocusedMonitor();
}

QBindable<NiriWorkspace*> NiriIpcQml::bindableFocusedWorkspace() {
	return NiriIpc::instance()->bindableFocusedWorkspace();
}

QBindable<NiriWindow*> NiriIpcQml::bindableActiveWindow() {
	return NiriIpc::instance()->bindableActiveWindow();
}

QBindable<bool> NiriIpcQml::bindableOverviewOpen() {
	return NiriIpc::instance()->bindableOverviewOpen();
}

QBindable<qint32> NiriIpcQml::bindableCurrentKeyboardLayout() {
	return NiriIpc::instance()->bindableCurrentKeyboardLayout();
}

QStringList NiriIpcQml::keyboardLayouts() { return NiriIpc::instance()->keyboardLayouts(); }

ObjectModel<NiriMonitor>* NiriIpcQml::monitors() { return NiriIpc::instance()->monitors(); }
ObjectModel<NiriWorkspace>* NiriIpcQml::workspaces() { return NiriIpc::instance()->workspaces(); }
ObjectModel<NiriWindow>* NiriIpcQml::windows() { return NiriIpc::instance()->windows(); }
ObjectModel<NiriCast>* NiriIpcQml::casts() { return NiriIpc::instance()->casts(); }

} // namespace qs::niri::ipc
