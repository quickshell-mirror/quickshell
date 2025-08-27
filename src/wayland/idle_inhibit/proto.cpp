#include "proto.hpp"

#include <private/qwaylandwindow_p.h>
#include <qwaylandclientextension.h>

namespace qs::wayland::idle_inhibit::impl {

IdleInhibitManager::IdleInhibitManager(): QWaylandClientExtensionTemplate(1) { this->initialize(); }

IdleInhibitManager* IdleInhibitManager::instance() {
	static auto* instance = new IdleInhibitManager(); // NOLINT
	return instance->isInitialized() ? instance : nullptr;
}

IdleInhibitor* IdleInhibitManager::createIdleInhibitor(QtWaylandClient::QWaylandWindow* surface) {
	return new IdleInhibitor(this->create_inhibitor(surface->surface()));
}

IdleInhibitor::~IdleInhibitor() {
	if (this->isInitialized()) this->destroy();
}

} // namespace qs::wayland::idle_inhibit::impl
