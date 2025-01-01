#include "surface.hpp"

#include <qtypes.h>
#include <qwayland-hyprland-surface-v1.h>
#include <wayland-hyprland-surface-v1-client-protocol.h>
#include <wayland-util.h>

namespace qs::hyprland::surface::impl {

HyprlandSurface::HyprlandSurface(::hyprland_surface_v1* surface)
    : QtWayland::hyprland_surface_v1(surface) {}

HyprlandSurface::~HyprlandSurface() { this->destroy(); }

void HyprlandSurface::setOpacity(qreal opacity) {
	this->set_opacity(wl_fixed_from_double(opacity));
}

} // namespace qs::hyprland::surface::impl
