#include "popupanchor.hpp"

#include <private/qhighdpiscaling_p.h>
#include <private/qwayland-xdg-shell.h>
#include <private/qwaylandwindow_p.h>
#include <private/wayland-xdg-shell-client-protocol.h>
#include <qvariant.h>
#include <qwindow.h>

#include "../core/popupanchor.hpp"
#include "../core/types.hpp"
#include "xdgshell.hpp"

using QtWaylandClient::QWaylandWindow;
using XdgPositioner = QtWayland::xdg_positioner;
using qs::wayland::xdg_shell::XdgWmBase;

void WaylandPopupPositioner::reposition(PopupAnchor* anchor, QWindow* window, bool onlyIfDirty) {

	auto* waylandWindow = dynamic_cast<QWaylandWindow*>(window->handle());
	auto* popupRole = waylandWindow ? waylandWindow->surfaceRole<::xdg_popup>() : nullptr;

	anchor->updateAnchor();

	// If a popup becomes invisble after creation ensure the _q properties will
	// be set and not ignored because the rest is the same.
	anchor->updatePlacement({popupRole != nullptr, 0}, window->size());

	if (onlyIfDirty && !anchor->isDirty()) return;
	anchor->markClean();

	if (popupRole) {
		auto* xdgWmBase = XdgWmBase::instance();

		if (xdgWmBase->QtWayland::xdg_wm_base::version() < XDG_POPUP_REPOSITION_SINCE_VERSION) {
			window->setVisible(false);
			WaylandPopupPositioner::setFlags(anchor, window);
			window->setVisible(true);
			return;
		}

		auto positioner = XdgPositioner(xdgWmBase->create_positioner());

		positioner.set_constraint_adjustment(anchor->adjustment().toInt());

		auto anchorRect = anchor->windowRect();

		if (auto* p = window->transientParent()) {
			anchorRect = QHighDpi::toNativePixels(anchorRect, p);
		}

		positioner
		    .set_anchor_rect(anchorRect.x(), anchorRect.y(), anchorRect.width(), anchorRect.height());

		XdgPositioner::anchor anchorFlag = XdgPositioner::anchor_none;
		switch (anchor->edges()) {
		case Edges::Top: anchorFlag = XdgPositioner::anchor_top; break;
		case Edges::Top | Edges::Right: anchorFlag = XdgPositioner::anchor_top_right; break;
		case Edges::Right: anchorFlag = XdgPositioner::anchor_right; break;
		case Edges::Bottom | Edges::Right: anchorFlag = XdgPositioner::anchor_bottom_right; break;
		case Edges::Bottom: anchorFlag = XdgPositioner::anchor_bottom; break;
		case Edges::Bottom | Edges::Left: anchorFlag = XdgPositioner::anchor_bottom_left; break;
		case Edges::Left: anchorFlag = XdgPositioner::anchor_left; break;
		case Edges::Top | Edges::Left: anchorFlag = XdgPositioner::anchor_top_left; break;
		default: break;
		}

		positioner.set_anchor(anchorFlag);

		XdgPositioner::gravity gravity = XdgPositioner::gravity_none;
		switch (anchor->gravity()) {
		case Edges::Top: gravity = XdgPositioner::gravity_top; break;
		case Edges::Top | Edges::Right: gravity = XdgPositioner::gravity_top_right; break;
		case Edges::Right: gravity = XdgPositioner::gravity_right; break;
		case Edges::Bottom | Edges::Right: gravity = XdgPositioner::gravity_bottom_right; break;
		case Edges::Bottom: gravity = XdgPositioner::gravity_bottom; break;
		case Edges::Bottom | Edges::Left: gravity = XdgPositioner::gravity_bottom_left; break;
		case Edges::Left: gravity = XdgPositioner::gravity_left; break;
		case Edges::Top | Edges::Left: gravity = XdgPositioner::gravity_top_left; break;
		default: break;
		}

		positioner.set_gravity(gravity);
		auto geometry = waylandWindow->geometry();
		positioner.set_size(geometry.width(), geometry.height());

		// Note: this needs to be set for the initial position as well but no compositor
		// supports it enough to test
		positioner.set_reactive();

		xdg_popup_reposition(popupRole, positioner.object(), 0);

		positioner.destroy();
	} else {
		WaylandPopupPositioner::setFlags(anchor, window);
	}
}

// Should be false but nobody supports set_reactive.
// This just tries its best when something like a bar gets resized.
bool WaylandPopupPositioner::shouldRepositionOnMove() const { return true; }

void WaylandPopupPositioner::setFlags(PopupAnchor* anchor, QWindow* window) {
	anchor->updateAnchor();
	auto anchorRect = anchor->windowRect();

	if (auto* p = window->transientParent()) {
		anchorRect = QHighDpi::toNativePixels(anchorRect, p);
	}

	// clang-format off
	window->setProperty("_q_waylandPopupConstraintAdjustment", anchor->adjustment().toInt());
	window->setProperty("_q_waylandPopupAnchorRect", anchorRect);
	window->setProperty("_q_waylandPopupAnchor", QVariant::fromValue(Edges::toQt(anchor->edges())));
	window->setProperty("_q_waylandPopupGravity", QVariant::fromValue(Edges::toQt(anchor->gravity())));
	// clang-format on
}

void installPopupPositioner() { PopupPositioner::setInstance(new WaylandPopupPositioner()); }
