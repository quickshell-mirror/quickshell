#include "proto.hpp"

#include <private/qwaylanddisplay_p.h>
#include <private/qwaylandinputdevice_p.h>
#include <private/qwaylandintegration_p.h>
#include <private/qwaylandwindow_p.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qwaylandclientextension.h>

#include "../../core/logcat.hpp"

namespace qs::wayland::shortcuts_inhibit::impl {

namespace {
QS_LOGGING_CATEGORY(logShortcutsInhibit, "quickshell.wayland.shortcuts_inhibit", QtWarningMsg);
}

ShortcutsInhibitManager::ShortcutsInhibitManager(): QWaylandClientExtensionTemplate(1) {
	this->initialize();
}

ShortcutsInhibitManager* ShortcutsInhibitManager::instance() {
	static auto* instance = new ShortcutsInhibitManager(); // NOLINT
	return instance->isInitialized() ? instance : nullptr;
}

ShortcutsInhibitor*
ShortcutsInhibitManager::createShortcutsInhibitor(QtWaylandClient::QWaylandWindow* surface) {
	auto* display = QtWaylandClient::QWaylandIntegration::instance()->display();
	auto* inputDevice = display->lastInputDevice();
	if (inputDevice == nullptr) inputDevice = display->defaultInputDevice();

	if (inputDevice == nullptr) {
		qCCritical(logShortcutsInhibit) << "Could not create shortcuts inhibitor: No seat.";
		return nullptr;
	}

	auto* inhibitor =
	    new ShortcutsInhibitor(this->inhibit_shortcuts(surface->surface(), inputDevice->object()));
	qCDebug(logShortcutsInhibit) << "Created inhibitor" << inhibitor;
	return inhibitor;
}

ShortcutsInhibitor::~ShortcutsInhibitor() {
	qCDebug(logShortcutsInhibit) << "Destroyed inhibitor" << this;
	if (this->isInitialized()) this->destroy();
}

} // namespace qs::wayland::shortcuts_inhibit::impl