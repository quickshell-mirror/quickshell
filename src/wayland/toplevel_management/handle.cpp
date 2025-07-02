#include "handle.hpp"
#include <cstddef>

#include <private/qwaylanddisplay_p.h>
#include <private/qwaylandinputdevice_p.h>
#include <private/qwaylandintegration_p.h>
#include <private/qwaylandscreen_p.h>
#include <private/qwaylandwindow_p.h>
#include <qcontainerfwd.h>
#include <qguiapplication.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qscreen.h>
#include <qtmetamacros.h>
#include <wayland-util.h>

#include "manager.hpp"
#include "qwayland-wlr-foreign-toplevel-management-unstable-v1.h"
#include "wayland-wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"

namespace qs::wayland::toplevel_management::impl {

QString ToplevelHandle::appId() const { return this->mAppId; }
QString ToplevelHandle::title() const { return this->mTitle; }
ToplevelHandle* ToplevelHandle::parent() const { return this->mParent; }
bool ToplevelHandle::activated() const { return this->mActivated; }
bool ToplevelHandle::maximized() const { return this->mMaximized; }
bool ToplevelHandle::minimized() const { return this->mMinimized; }
bool ToplevelHandle::fullscreen() const { return this->mFullscreen; }

void ToplevelHandle::activate() {
	auto* display = QtWaylandClient::QWaylandIntegration::instance()->display();
	auto* inputDevice = display->lastInputDevice();
	if (inputDevice == nullptr) return;
	this->QtWayland::zwlr_foreign_toplevel_handle_v1::activate(inputDevice->object());
}

void ToplevelHandle::setMaximized(bool maximized) {
	if (maximized) this->set_maximized();
	else this->unset_maximized();
}

void ToplevelHandle::setMinimized(bool minimized) {
	if (minimized) this->set_minimized();
	else this->unset_minimized();
}

void ToplevelHandle::setFullscreen(bool fullscreen) {
	if (fullscreen) this->set_fullscreen(nullptr);
	else this->unset_fullscreen();
}

void ToplevelHandle::fullscreenOn(QScreen* screen) {
	auto* waylandScreen = dynamic_cast<QtWaylandClient::QWaylandScreen*>(screen->handle());
	this->set_fullscreen(waylandScreen != nullptr ? waylandScreen->output() : nullptr);
}

void ToplevelHandle::setRectangle(QWindow* window, QRect rect) {
	if (window == nullptr) {
		// will be cleared by the compositor if the surface is destroyed
		if (this->rectWindow != nullptr) {
			auto* waylandWindow =
			    dynamic_cast<QtWaylandClient::QWaylandWindow*>(this->rectWindow->handle());

			if (waylandWindow != nullptr) {
				this->set_rectangle(waylandWindow->surface(), 0, 0, 0, 0);
			}
		}

		QObject::disconnect(this->rectWindow, nullptr, this, nullptr);
		this->rectWindow = nullptr;
		return;
	}

	if (this->rectWindow != window) {
		if (this->rectWindow != nullptr) {
			QObject::disconnect(this->rectWindow, nullptr, this, nullptr);
		}

		this->rectWindow = window;
		QObject::connect(window, &QObject::destroyed, this, &ToplevelHandle::onRectWindowDestroyed);
	}

	if (auto* waylandWindow = dynamic_cast<QtWaylandClient::QWaylandWindow*>(window->handle())) {
		this->set_rectangle(waylandWindow->surface(), rect.x(), rect.y(), rect.width(), rect.height());
	} else {
		QObject::connect(window, &QWindow::visibleChanged, this, [this, window, rect]() {
			if (window->isVisible()) {
				if (window->handle() == nullptr) {
					window->create();
				}

				auto* waylandWindow = dynamic_cast<QtWaylandClient::QWaylandWindow*>(window->handle());
				this->set_rectangle(
				    waylandWindow->surface(),
				    rect.x(),
				    rect.y(),
				    rect.width(),
				    rect.height()
				);
			}
		});
	}
}

void ToplevelHandle::onRectWindowDestroyed() { this->rectWindow = nullptr; }

void ToplevelHandle::zwlr_foreign_toplevel_handle_v1_done() {
	qCDebug(logToplevelManagement) << this << "got done";
	auto wasReady = this->isReady;
	this->isReady = true;

	if (!wasReady) {
		emit this->ready();
	}
}

void ToplevelHandle::zwlr_foreign_toplevel_handle_v1_closed() {
	qCDebug(logToplevelManagement) << this << "closed";
	this->destroy();
	emit this->closed();
	delete this;
}

void ToplevelHandle::zwlr_foreign_toplevel_handle_v1_app_id(const QString& appId) {
	qCDebug(logToplevelManagement) << this << "got appid" << appId;
	this->mAppId = appId;
	emit this->appIdChanged();
}

void ToplevelHandle::zwlr_foreign_toplevel_handle_v1_title(const QString& title) {
	qCDebug(logToplevelManagement) << this << "got toplevel" << title;
	this->mTitle = title;
	emit this->titleChanged();
}

void ToplevelHandle::zwlr_foreign_toplevel_handle_v1_state(wl_array* stateArray) {
	auto activated = false;
	auto maximized = false;
	auto minimized = false;
	auto fullscreen = false;

	// wl_array_for_each is illegal in C++ so it is manually expanded.
	auto* state = static_cast<::zwlr_foreign_toplevel_handle_v1_state*>(stateArray->data);
	auto size = stateArray->size / sizeof(::zwlr_foreign_toplevel_handle_v1_state);
	for (size_t i = 0; i < size; i++) {
		auto flag = state[i]; // NOLINT
		switch (flag) {
		case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED: activated = true; break;
		case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED: maximized = true; break;
		case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED: minimized = true; break;
		case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN: fullscreen = true; break;
		}
	}

	qCDebug(logToplevelManagement) << this << "got state update - activated:" << activated
	                               << "maximized:" << maximized << "minimized:" << minimized
	                               << "fullscreen:" << fullscreen;

	if (activated != this->mActivated) {
		this->mActivated = activated;
		emit this->activatedChanged();
	}

	if (maximized != this->mMaximized) {
		this->mMaximized = maximized;
		emit this->maximizedChanged();
	}

	if (minimized != this->mMinimized) {
		this->mMinimized = minimized;
		emit this->minimizedChanged();
	}

	if (fullscreen != this->mFullscreen) {
		this->mFullscreen = fullscreen;
		emit this->fullscreenChanged();
	}
}

void ToplevelHandle::zwlr_foreign_toplevel_handle_v1_output_enter(wl_output* output) {
	qCDebug(logToplevelManagement) << this << "got output enter" << output;
	this->visibleScreens.addOutput(output);
}

void ToplevelHandle::zwlr_foreign_toplevel_handle_v1_output_leave(wl_output* output) {
	qCDebug(logToplevelManagement) << this << "got output leave" << output;
	this->visibleScreens.removeOutput(output);
}

void ToplevelHandle::zwlr_foreign_toplevel_handle_v1_parent(
    ::zwlr_foreign_toplevel_handle_v1* parent
) {
	auto* handle = ToplevelManager::instance()->handleFor(parent);
	qCDebug(logToplevelManagement) << this << "got parent" << handle;

	if (handle != this->mParent) {
		if (this->mParent != nullptr) {
			QObject::disconnect(this->mParent, nullptr, this, nullptr);
		}

		this->mParent = handle;

		if (handle != nullptr) {
			QObject::connect(handle, &ToplevelHandle::closed, this, &ToplevelHandle::onParentClosed);
		}

		emit this->parentChanged();
	}
}

void ToplevelHandle::onParentClosed() {
	this->mParent = nullptr;
	emit this->parentChanged();
}

} // namespace qs::wayland::toplevel_management::impl
