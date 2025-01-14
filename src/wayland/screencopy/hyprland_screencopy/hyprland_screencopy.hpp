#pragma once

#include <qwayland-hyprland-toplevel-export-v1.h>
#include <qwaylandclientextension.h>

#include "../../toplevel_management/handle.hpp"
#include "../manager.hpp"

namespace qs::wayland::screencopy::hyprland {

class HyprlandScreencopyManager
    : public QWaylandClientExtensionTemplate<HyprlandScreencopyManager>
    , public QtWayland::hyprland_toplevel_export_manager_v1 {
public:
	ScreencopyContext*
	captureToplevel(toplevel_management::impl::ToplevelHandle* handle, bool paintCursors);

	static HyprlandScreencopyManager* instance();

private:
	explicit HyprlandScreencopyManager();

	friend class HyprlandScreencopyContext;
};

} // namespace qs::wayland::screencopy::hyprland
