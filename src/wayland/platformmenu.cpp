#include "platformmenu.hpp"

#include <private/qwayland-xdg-shell.h>
#include <qmargins.h>
#include <qnamespace.h>
#include <qsize.h>
#include <qvariant.h>
#include <qwindow.h>

#include "../core/platformmenu.hpp"
#include "../core/platformmenu_p.hpp"

using namespace qs::menu::platform;

namespace {

// fixes positioning of submenus when hitting screen edges
void platformMenuHook(PlatformMenuQMenu* menu) {
	auto* window = menu->windowHandle();

	Qt::Edges anchor;
	Qt::Edges gravity;

	if (auto* containingMenu = menu->containingMenu) {
		auto geom = containingMenu->actionGeometry(menu->menuAction());

		// Forces action rects to be refreshed. Without this the geometry is intermittently null.
		containingMenu->sizeHint();

		// use the first action to find the offsets relative to the containing window
		auto baseGeom = containingMenu->actionGeometry(containingMenu->actions().first());
		geom += QMargins(0, baseGeom.top(), 0, baseGeom.top());

		window->setProperty("_q_waylandPopupAnchorRect", geom);

		auto sideEdge = menu->isRightToLeft() ? Qt::LeftEdge : Qt::RightEdge;
		anchor = Qt::TopEdge | sideEdge;
		gravity = Qt::BottomEdge | sideEdge;
	} else if (auto* parent = window->transientParent()) {
		// abort if already set by a PopupAnchor
		if (window->property("_q_waylandPopupAnchorRect").isValid()) return;

		// The menu geometry will be adjusted to flip internally by qt already, but it ends up off by
		// one pixel which causes the compositor to also flip which results in the menu being placed
		// left of the edge by its own width. To work around this the intended position is stored prior
		// to tampering by qt.
		auto anchorRect = QRect(menu->targetPosition - parent->geometry().topLeft(), QSize(1, 1));
		window->setProperty("_q_waylandPopupAnchorRect", anchorRect);

		anchor = Qt::BottomEdge | Qt::RightEdge;
		gravity = Qt::BottomEdge | Qt::RightEdge;
	}

	window->setProperty("_q_waylandPopupAnchor", QVariant::fromValue(anchor));
	window->setProperty("_q_waylandPopupGravity", QVariant::fromValue(gravity));

	auto constraintAdjustment = QtWayland::xdg_positioner::constraint_adjustment_flip_x
	                          | QtWayland::xdg_positioner::constraint_adjustment_flip_y
	                          | QtWayland::xdg_positioner::constraint_adjustment_slide_x
	                          | QtWayland::xdg_positioner::constraint_adjustment_slide_y
	                          | QtWayland::xdg_positioner::constraint_adjustment_resize_x
	                          | QtWayland::xdg_positioner::constraint_adjustment_resize_y;

	window->setProperty("_q_waylandPopupConstraintAdjustment", constraintAdjustment);
}

} // namespace

void installPlatformMenuHook() { PlatformMenuEntry::registerCreationHook(&platformMenuHook); }
