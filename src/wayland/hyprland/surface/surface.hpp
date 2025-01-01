#pragma once

#include <qobject.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwayland-hyprland-surface-v1.h>
#include <wayland-hyprland-surface-v1-client-protocol.h>

namespace qs::hyprland::surface::impl {

class HyprlandSurface: public QtWayland::hyprland_surface_v1 {
public:
	explicit HyprlandSurface(::hyprland_surface_v1* surface);
	~HyprlandSurface() override;
	Q_DISABLE_COPY_MOVE(HyprlandSurface);

	void setOpacity(qreal opacity);
};

} // namespace qs::hyprland::surface::impl
