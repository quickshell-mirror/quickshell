#include "manager.hpp"

#include <qobject.h>

#include "build.hpp"

#if SCREENCOPY_ICC || SCREENCOPY_WLR
#include "../../core/qmlscreen.hpp"
#endif

#if SCREENCOPY_ICC
#include "image_copy_capture/image_copy_capture.hpp"
#endif

#if SCREENCOPY_WLR
#include "wlr_screencopy/wlr_screencopy.hpp"
#endif

#if SCREENCOPY_HYPRLAND_TOPLEVEL
#include "../toplevel_management/qml.hpp"
#include "hyprland_screencopy/hyprland_screencopy.hpp"
#endif

namespace qs::wayland::screencopy {

ScreencopyContext* ScreencopyManager::createContext(QObject* object, bool paintCursors) {
	if (auto* screen = qobject_cast<QuickshellScreenInfo*>(object)) {
#if SCREENCOPY_ICC
		{
			auto* manager = icc::IccOutputSourceManager::instance();
			if (manager->isActive()) {
				return manager->captureOutput(screen->screen, paintCursors);
			}
		}
#endif
#if SCREENCOPY_WLR
		{
			auto* manager = wlr::WlrScreencopyManager::instance();
			if (manager->isActive()) {
				return manager->captureOutput(screen->screen, paintCursors);
			}
		}
#endif
#if SCREENCOPY_HYPRLAND_TOPLEVEL
	} else if (auto* toplevel = qobject_cast<toplevel_management::Toplevel*>(object)) {
		auto* manager = hyprland::HyprlandScreencopyManager::instance();
		if (manager->isActive()) {
			return manager->captureToplevel(toplevel->implHandle(), paintCursors);
		}
#endif
	}

	return nullptr;
}

} // namespace qs::wayland::screencopy
