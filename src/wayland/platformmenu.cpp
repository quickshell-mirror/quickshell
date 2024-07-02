#include "platformmenu.hpp"

#include <private/qwayland-xdg-shell.h>
#include <qmargins.h>
#include <qwindow.h>

#include "../core/platformmenu.hpp"

using namespace qs::menu::platform;
using namespace QtWayland;

// fixes positioning of submenus when hitting screen edges
void platformMenuHook(PlatformMenuQMenu* menu) {
	auto* window = menu->windowHandle();

	auto constraintAdjustment = QtWayland::xdg_positioner::constraint_adjustment_flip_x
	                          | QtWayland::xdg_positioner::constraint_adjustment_flip_y;

	window->setProperty("_q_waylandPopupConstraintAdjustment", constraintAdjustment);

	if (auto* containingMenu = menu->containingMenu) {
		auto geom = containingMenu->actionGeometry(menu->menuAction());

		// use the first action to find the offsets relative to the containing window
		auto baseGeom = containingMenu->actionGeometry(containingMenu->actions().first());
		geom += QMargins(0, baseGeom.top(), 0, baseGeom.top());

		window->setProperty("_q_waylandPopupAnchorRect", geom);
	}
}

void installPlatformMenuHook() { PlatformMenuEntry::registerCreationHook(&platformMenuHook); }
