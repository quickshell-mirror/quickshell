#pragma once

#include <private/qwaylandwindow_p.h>
#include <qwayland-hyprland-surface-v1.h>
#include <qwaylandclientextension.h>

#include "surface.hpp"

namespace qs::hyprland::surface::impl {

class HyprlandSurfaceManager
    : public QWaylandClientExtensionTemplate<HyprlandSurfaceManager>
    , public QtWayland::hyprland_surface_manager_v1 {
public:
	explicit HyprlandSurfaceManager();

	HyprlandSurface* createHyprlandExtension(QtWaylandClient::QWaylandWindow* surface);

	static HyprlandSurfaceManager* instance();
};

} // namespace qs::hyprland::surface::impl
