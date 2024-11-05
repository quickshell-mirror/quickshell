#include "grab.hpp"

#include <private/qwaylandwindow_p.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qwindow.h>
#include <wayland-hyprland-focus-grab-v1-client-protocol.h>

namespace qs::hyprland::focus_grab {

FocusGrab::FocusGrab(::hyprland_focus_grab_v1* grab) { this->init(grab); }

FocusGrab::~FocusGrab() {
	if (this->isInitialized()) {
		this->destroy();
	}
}

bool FocusGrab::isActive() const { return this->active; }

void FocusGrab::addWindow(QWindow* window) {
	auto tryAddWayland = [this](QWaylandWindow* waylandWindow) {
		if (waylandWindow->surface()) {
			this->addWaylandWindow(waylandWindow);
			this->sync();
		} else {
			QObject::connect(
			    waylandWindow,
			    &QWaylandWindow::surfaceCreated,
			    this,
			    [this, waylandWindow]() {
				    this->addWaylandWindow(waylandWindow);
				    this->sync();
			    }
			);
		}
	};

	if (auto* waylandWindow = dynamic_cast<QWaylandWindow*>(window->handle())) {
		tryAddWayland(waylandWindow);
	} else {
		QObject::connect(window, &QWindow::visibleChanged, this, [window, tryAddWayland]() {
			if (window->isVisible()) {
				if (window->handle() == nullptr) {
					window->create();
				}

				auto* waylandWindow = dynamic_cast<QWaylandWindow*>(window->handle());
				tryAddWayland(waylandWindow);
			}
		});
	}
}

void FocusGrab::removeWindow(QWindow* window) {
	QObject::disconnect(window, nullptr, this, nullptr);

	if (auto* waylandWindow = dynamic_cast<QWaylandWindow*>(window->handle())) {
		this->pendingAdditions.removeAll(waylandWindow);
		this->remove_surface(waylandWindow->surface());
		this->commitRequired = true;
	}
}

void FocusGrab::addWaylandWindow(QWaylandWindow* window) {
	this->add_surface(window->surface());
	this->pendingAdditions.append(window);
	this->commitRequired = true;
}

void FocusGrab::sync() {
	if (this->transactionActive) return;

	if (this->commitRequired) {
		this->commit();
		this->commitRequired = false;

		// the protocol will always send cleared() when the grab is deactivated,
		// even if it was due to window destruction, so we don't need to track it.
		if (!this->pendingAdditions.isEmpty()) {
			this->pendingAdditions.clear();

			if (!this->active) {
				this->active = true;
				emit this->activated();
			}
		}
	}
}

void FocusGrab::startTransaction() { this->transactionActive = true; }

void FocusGrab::completeTransaction() {
	this->transactionActive = false;
	this->sync();
}

void FocusGrab::hyprland_focus_grab_v1_cleared() {
	this->active = false;
	emit this->cleared();
}

} // namespace qs::hyprland::focus_grab
