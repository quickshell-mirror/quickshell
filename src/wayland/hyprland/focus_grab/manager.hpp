#pragma once

#include <qwayland-hyprland-focus-grab-v1.h>
#include <qwaylandclientextension.h>

namespace qs::hyprland::focus_grab {
using HyprlandFocusGrabManager = QtWayland::hyprland_focus_grab_manager_v1;
class FocusGrab;

class FocusGrabManager
    : public QWaylandClientExtensionTemplate<FocusGrabManager>
    , public HyprlandFocusGrabManager {
public:
	explicit FocusGrabManager();

	[[nodiscard]] bool available() const;
	[[nodiscard]] FocusGrab* createGrab();

	static FocusGrabManager* instance();
};

} // namespace qs::hyprland::focus_grab
