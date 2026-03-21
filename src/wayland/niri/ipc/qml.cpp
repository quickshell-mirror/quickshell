#include "qml.hpp"

#include <qobject.h>
#include <qproperty.h>

#include "../../../core/model.hpp"
#include "connection.hpp"
#include "output.hpp"
#include "window.hpp"
#include "workspace.hpp"

namespace qs::niri::ipc {

NiriIpcQml::NiriIpcQml() {
	auto* instance = NiriIpc::instance();

	QObject::connect(instance, &NiriIpc::rawEvent, this, &NiriIpcQml::rawEvent);

	QObject::connect(
	    instance,
	    &NiriIpc::focusedWindowChanged,
	    this,
	    &NiriIpcQml::focusedWindowChanged
	);

	QObject::connect(
	    instance,
	    &NiriIpc::overviewActiveChanged,
	    this,
	    &NiriIpcQml::overviewActiveChanged
	);

	QObject::connect(
	    instance,
	    &NiriIpc::keyboardLayoutsChanged,
	    this,
	    &NiriIpcQml::keyboardLayoutsChanged
	);

	QObject::connect(
	    instance,
	    &NiriIpc::keyboardLayoutSwitched,
	    this,
	    &NiriIpcQml::keyboardLayoutSwitched
	);

	QObject::connect(
	    instance,
	    &NiriIpc::workspacesUpdated,
	    this,
	    &NiriIpcQml::workspacesUpdated
	);

	QObject::connect(
	    instance,
	    &NiriIpc::windowsUpdated,
	    this,
	    &NiriIpcQml::windowsUpdated
	);

	QObject::connect(
	    instance,
	    &NiriIpc::outputsUpdated,
	    this,
	    &NiriIpcQml::outputsUpdated
	);
}

void NiriIpcQml::dispatch(const QStringList& args) {
	NiriIpc::instance()->dispatch(args);
}

void NiriIpcQml::refreshOutputs() { NiriIpc::instance()->refreshOutputs(); }
void NiriIpcQml::refreshWorkspaces() { NiriIpc::instance()->refreshWorkspaces(); }
void NiriIpcQml::refreshWindows() { NiriIpc::instance()->refreshWindows(); }

QString NiriIpcQml::socketPath() { return NiriIpc::instance()->socketPath(); }

QBindable<NiriWindow*> NiriIpcQml::bindableFocusedWindow() {
	return NiriIpc::instance()->bindableFocusedWindow();
}

QBindable<bool> NiriIpcQml::bindableOverviewActive() {
	return NiriIpc::instance()->bindableOverviewActive();
}

QBindable<QStringList> NiriIpcQml::bindableKeyboardLayoutNames() {
	return NiriIpc::instance()->bindableKeyboardLayoutNames();
}

QBindable<qint32> NiriIpcQml::bindableCurrentKeyboardLayoutIndex() {
	return NiriIpc::instance()->bindableCurrentKeyboardLayoutIndex();
}

QString NiriIpcQml::currentKeyboardLayoutName() {
	auto names = NiriIpc::instance()->bindableKeyboardLayoutNames().value();
	auto idx = NiriIpc::instance()->bindableCurrentKeyboardLayoutIndex().value();
	if (idx >= 0 && idx < names.size()) {
		return names.at(idx);
	}
	return {};
}

ObjectModel<NiriOutput>* NiriIpcQml::outputs() {
	return NiriIpc::instance()->outputs();
}

ObjectModel<NiriWorkspace>* NiriIpcQml::workspaces() {
	return NiriIpc::instance()->workspaces();
}

ObjectModel<NiriWindow>* NiriIpcQml::windows() {
	return NiriIpc::instance()->windows();
}

} // namespace qs::niri::ipc
