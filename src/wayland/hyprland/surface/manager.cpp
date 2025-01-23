#include "manager.hpp"

#include <private/qwaylandwindow_p.h>
#include <qwaylandclientextension.h>

#include "surface.hpp"

namespace qs::hyprland::surface::impl {

HyprlandSurfaceManager::HyprlandSurfaceManager(): QWaylandClientExtensionTemplate(2) {
	this->initialize();
}

HyprlandSurface*
HyprlandSurfaceManager::createHyprlandExtension(QtWaylandClient::QWaylandWindow* surface) {
	return new HyprlandSurface(this->get_hyprland_surface(surface->surface()), surface);
}

HyprlandSurfaceManager* HyprlandSurfaceManager::instance() {
	static auto* instance = new HyprlandSurfaceManager();
	return instance;
}

} // namespace qs::hyprland::surface::impl
