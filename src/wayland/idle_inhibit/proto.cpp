#include "proto.hpp"

#include <private/qwaylandwindow_p.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qwaylandclientextension.h>

#include "../../core/logcat.hpp"

namespace qs::wayland::idle_inhibit::impl {

namespace {
QS_LOGGING_CATEGORY(logIdleInhibit, "quickshell.wayland.idle_inhibit", QtWarningMsg);
}

IdleInhibitManager::IdleInhibitManager(): QWaylandClientExtensionTemplate(1) { this->initialize(); }

IdleInhibitManager* IdleInhibitManager::instance() {
	static auto* instance = new IdleInhibitManager(); // NOLINT
	return instance->isInitialized() ? instance : nullptr;
}

IdleInhibitor* IdleInhibitManager::createIdleInhibitor(QtWaylandClient::QWaylandWindow* surface) {
	auto* inhibitor = new IdleInhibitor(this->create_inhibitor(surface->surface()));
	qCDebug(logIdleInhibit) << "Created inhibitor" << inhibitor;
	return inhibitor;
}

IdleInhibitor::~IdleInhibitor() {
	qCDebug(logIdleInhibit) << "Destroyed inhibitor" << this;
	if (this->isInitialized()) this->destroy();
}

} // namespace qs::wayland::idle_inhibit::impl
