#include "qml.hpp"
#include <memory>

#include <private/qwaylandwindow_p.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlinfo.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwindow.h>

#include "../../../window/proxywindow.hpp"
#include "../../../window/windowinterface.hpp"
#include "../../util.hpp"
#include "manager.hpp"
#include "surface.hpp"

using QtWaylandClient::QWaylandWindow;

namespace qs::hyprland::surface {

HyprlandWindow* HyprlandWindow::qmlAttachedProperties(QObject* object) {
	auto* proxyWindow = qobject_cast<ProxyWindowBase*>(object);

	if (!proxyWindow) {
		if (auto* iface = qobject_cast<WindowInterface*>(object)) {
			proxyWindow = iface->proxyWindow();
		}
	}

	if (!proxyWindow) return nullptr;
	return new HyprlandWindow(proxyWindow);
}

HyprlandWindow::HyprlandWindow(ProxyWindowBase* window): QObject(nullptr), proxyWindow(window) {
	QObject::connect(
	    window,
	    &ProxyWindowBase::windowConnected,
	    this,
	    &HyprlandWindow::onWindowConnected
	);

	QObject::connect(window, &QObject::destroyed, this, &HyprlandWindow::onProxyWindowDestroyed);

	if (window->backingWindow()) {
		this->onWindowConnected();
	}
}

qreal HyprlandWindow::opacity() const { return this->mOpacity; }

void HyprlandWindow::setOpacity(qreal opacity) {
	if (opacity == this->mOpacity) return;

	if (opacity < 0.0 || opacity > 1.0) {
		qmlWarning(this
		) << "Cannot set HyprlandWindow.opacity to a value larger than 1.0 or smaller than 0.0";
		return;
	}

	this->mOpacity = opacity;

	if (this->surface) {
		this->surface->setOpacity(opacity);
		qs::wayland::util::scheduleCommit(this->mWaylandWindow);
	}

	emit this->opacityChanged();
}

void HyprlandWindow::onWindowConnected() {
	this->mWindow = this->proxyWindow->backingWindow();
	// disconnected by destructor
	QObject::connect(
	    this->mWindow,
	    &QWindow::visibleChanged,
	    this,
	    &HyprlandWindow::onWindowVisibleChanged
	);

	this->onWindowVisibleChanged();
}

void HyprlandWindow::onWindowVisibleChanged() {
	if (this->mWindow->isVisible()) {
		if (!this->mWindow->handle()) {
			this->mWindow->create();
		}

		this->mWaylandWindow = dynamic_cast<QWaylandWindow*>(this->mWindow->handle());

		if (this->mWaylandWindow) {
			// disconnected by destructor

			QObject::connect(
			    this->mWaylandWindow,
			    &QWaylandWindow::surfaceCreated,
			    this,
			    &HyprlandWindow::onWaylandSurfaceCreated
			);

			QObject::connect(
			    this->mWaylandWindow,
			    &QWaylandWindow::surfaceDestroyed,
			    this,
			    &HyprlandWindow::onWaylandSurfaceDestroyed
			);

			if (this->mWaylandWindow->surface()) {
				this->onWaylandSurfaceCreated();
			}
		}
	}
}

void HyprlandWindow::onWaylandSurfaceCreated() {
	auto* manager = impl::HyprlandSurfaceManager::instance();

	if (!manager->isActive()) {
		qWarning() << "The active compositor does not support the hyprland_surface_v1 protocol. "
		              "HyprlandWindow will not work.";
		return;
	}

	auto* ext = manager->createHyprlandExtension(this->mWaylandWindow);
	this->surface = std::unique_ptr<impl::HyprlandSurface>(ext);

	if (this->mOpacity != 1.0) {
		this->surface->setOpacity(this->mOpacity);
		qs::wayland::util::scheduleCommit(this->mWaylandWindow);
	}
}

void HyprlandWindow::onWaylandSurfaceDestroyed() {
	this->surface = nullptr;

	if (!this->proxyWindow) {
		this->deleteLater();
	}
}

void HyprlandWindow::onProxyWindowDestroyed() {
	// Don't delete the HyprlandWindow, and therefore the impl::HyprlandSurface until the wl_surface is destroyed.
	// Deleting it when the proxy window is deleted will cause a full opacity frame between the destruction of the
	// hyprland_surface_v1 and wl_surface objects.

	if (this->surface == nullptr) {
		this->proxyWindow = nullptr;
		this->deleteLater();
	}
}

} // namespace qs::hyprland::surface
