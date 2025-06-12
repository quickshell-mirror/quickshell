#include "surface.hpp"
#include <cmath>

#include <private/qwaylanddisplay_p.h>
#include <private/qwaylandintegration_p.h>
#include <qlogging.h>
#include <qregion.h>
#include <qtypes.h>
#include <qwayland-hyprland-surface-v1.h>
#include <wayland-client-protocol.h>
#include <wayland-hyprland-surface-v1-client-protocol.h>
#include <wayland-util.h>

namespace qs::hyprland::surface::impl {

HyprlandSurface::HyprlandSurface(
    ::hyprland_surface_v1* surface,
    QtWaylandClient::QWaylandWindow* backer
)
    : QtWayland::hyprland_surface_v1(surface)
    , backerSurface(backer->surface()) {}

HyprlandSurface::~HyprlandSurface() { this->destroy(); }

bool HyprlandSurface::surfaceEq(wl_surface* surface) const {
	return surface == this->backerSurface;
}

void HyprlandSurface::setOpacity(qreal opacity) {
	this->set_opacity(wl_fixed_from_double(opacity));
}

void HyprlandSurface::setVisibleRegion(const QRegion& region) {
	if (this->version() < HYPRLAND_SURFACE_V1_SET_VISIBLE_REGION_SINCE_VERSION) {
		qWarning() << "Cannot set hyprland surface visible region: compositor does not support "
		              "hyprland_surface_v1.set_visible_region";
		return;
	}

	if (region.isEmpty()) {
		this->set_visible_region(nullptr);
	} else {
		static const auto* waylandIntegration = QtWaylandClient::QWaylandIntegration::instance();
		auto* display = waylandIntegration->display();

		auto* wlRegion = display->createRegion(region);
		this->set_visible_region(wlRegion);
		wl_region_destroy(wlRegion); // NOLINT(misc-include-cleaner)
	}
}

} // namespace qs::hyprland::surface::impl
