#pragma once

#include <private/qwaylandwindow_p.h>
#include <qobject.h>
#include <qregion.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwayland-hyprland-surface-v1.h>
#include <wayland-client-protocol.h>
#include <wayland-hyprland-surface-v1-client-protocol.h>

namespace qs::hyprland::surface::impl {

class HyprlandSurface: public QtWayland::hyprland_surface_v1 {
public:
	explicit HyprlandSurface(::hyprland_surface_v1* surface, QtWaylandClient::QWaylandWindow* backer);
	~HyprlandSurface() override;
	Q_DISABLE_COPY_MOVE(HyprlandSurface);

	[[nodiscard]] bool surfaceEq(wl_surface* surface) const;

	void setOpacity(qreal opacity);
	void setVisibleRegion(const QRegion& region);

private:
	wl_surface* backerSurface = nullptr;
};

} // namespace qs::hyprland::surface::impl
