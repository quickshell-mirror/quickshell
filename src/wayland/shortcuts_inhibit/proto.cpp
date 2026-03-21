#include "proto.hpp"

#include <private/qwaylanddisplay_p.h>
#include <private/qwaylandinputdevice_p.h>
#include <private/qwaylandintegration_p.h>
#include <private/qwaylandwindow_p.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qpair.h>
#include <qwaylandclientextension.h>

#include "../../core/logcat.hpp"

namespace qs::wayland::shortcuts_inhibit::impl {

QS_LOGGING_CATEGORY(logShortcutsInhibit, "quickshell.wayland.shortcuts_inhibit", QtWarningMsg);

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

	auto* wlSurface = surface->surface();

	if (this->inhibitors.contains(wlSurface)) {
		auto& pair = this->inhibitors[wlSurface];
		pair.second++;
		qCDebug(logShortcutsInhibit) << "Reusing existing inhibitor" << pair.first << "for surface"
		                             << wlSurface << "- refcount:" << pair.second;
		return pair.first;
	}

	auto* inhibitor =
	    new ShortcutsInhibitor(this->inhibit_shortcuts(wlSurface, inputDevice->object()), wlSurface);
	this->inhibitors.insert(wlSurface, qMakePair(inhibitor, 1));
	qCDebug(logShortcutsInhibit) << "Created inhibitor" << inhibitor << "for surface" << wlSurface;
	return inhibitor;
}

void ShortcutsInhibitManager::unrefShortcutsInhibitor(ShortcutsInhibitor* inhibitor) {
	if (!inhibitor) return;

	auto* surface = inhibitor->surface();
	if (!this->inhibitors.contains(surface)) return;

	auto& pair = this->inhibitors[surface];
	pair.second--;
	qCDebug(logShortcutsInhibit) << "Decremented refcount for inhibitor" << inhibitor
	                             << "- refcount:" << pair.second;

	if (pair.second <= 0) {
		qCDebug(logShortcutsInhibit) << "Refcount reached 0, destroying inhibitor" << inhibitor;
		this->inhibitors.remove(surface);
		delete inhibitor;
	}
}

ShortcutsInhibitor::~ShortcutsInhibitor() {
	qCDebug(logShortcutsInhibit) << "Destroying inhibitor" << this << "for surface" << this->mSurface;
	if (this->isInitialized()) this->destroy();
}

void ShortcutsInhibitor::zwp_keyboard_shortcuts_inhibitor_v1_active() {
	qCDebug(logShortcutsInhibit) << "Inhibitor became active" << this;
	this->bActive = true;
}

void ShortcutsInhibitor::zwp_keyboard_shortcuts_inhibitor_v1_inactive() {
	qCDebug(logShortcutsInhibit) << "Inhibitor became inactive" << this;
	this->bActive = false;
}

} // namespace qs::wayland::shortcuts_inhibit::impl