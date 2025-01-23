#include "qml.hpp"
#include <memory>

#include <private/qhighdpiscaling_p.h>
#include <private/qwaylandwindow_p.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlinfo.h>
#include <qregion.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>
#include <qwindow.h>

#include "../../../core/region.hpp"
#include "../../../window/proxywindow.hpp"
#include "../../../window/windowinterface.hpp"
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

	QObject::connect(window, &ProxyWindowBase::polished, this, &HyprlandWindow::onWindowPolished);

	QObject::connect(
	    window,
	    &ProxyWindowBase::devicePixelRatioChanged,
	    this,
	    &HyprlandWindow::updateVisibleMask
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

	if (this->surface && this->proxyWindow) {
		this->pendingPolish.opacity = true;
		this->proxyWindow->schedulePolish();
	}

	emit this->opacityChanged();
}

PendingRegion* HyprlandWindow::visibleMask() const { return this->mVisibleMask; }

void HyprlandWindow::setVisibleMask(PendingRegion* mask) {
	if (mask == this->mVisibleMask) return;

	if (this->mVisibleMask) {
		QObject::disconnect(this->mVisibleMask, nullptr, this, nullptr);
	}

	this->mVisibleMask = mask;

	if (mask) {
		QObject::connect(mask, &QObject::destroyed, this, &HyprlandWindow::onVisibleMaskDestroyed);
		QObject::connect(mask, &PendingRegion::changed, this, &HyprlandWindow::updateVisibleMask);
	}

	this->updateVisibleMask();
	emit this->visibleMaskChanged();
}

void HyprlandWindow::onVisibleMaskDestroyed() {
	this->mVisibleMask = nullptr;
	this->updateVisibleMask();
	emit this->visibleMaskChanged();
}

void HyprlandWindow::updateVisibleMask() {
	if (!this->surface || !this->proxyWindow) return;

	this->pendingPolish.visibleMask = true;
	this->proxyWindow->schedulePolish();
}

void HyprlandWindow::onWindowPolished() {
	if (!this->surface) return;

	if (this->pendingPolish.opacity) {
		this->surface->setOpacity(this->mOpacity);
		this->pendingPolish.opacity = false;
	}

	if (this->pendingPolish.visibleMask) {
		QRegion mask;
		if (this->mVisibleMask != nullptr) {
			mask =
			    this->mVisibleMask->applyTo(QRect(0, 0, this->mWindow->width(), this->mWindow->height()));
		}

		auto dpr = this->proxyWindow->devicePixelRatio();
		if (dpr != 1.0) {
			mask = QHighDpi::scale(mask, dpr);
		}

		if (mask.isEmpty() && this->mVisibleMask) {
			mask = QRect(-1, -1, 1, 1);
		}

		this->surface->setVisibleRegion(mask);
		this->pendingPolish.visibleMask = false;
	}
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
	}

	auto* window = dynamic_cast<QWaylandWindow*>(this->mWindow->handle());
	if (window == this->mWaylandWindow) return;

	if (this->mWaylandWindow) {
		QObject::disconnect(this->mWaylandWindow, nullptr, this, nullptr);
	}

	this->mWaylandWindow = window;
	if (!window) return;

	QObject::connect(
	    this->mWaylandWindow,
	    &QObject::destroyed,
	    this,
	    &HyprlandWindow::onWaylandWindowDestroyed
	);

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

void HyprlandWindow::onWaylandWindowDestroyed() { this->mWaylandWindow = nullptr; }

void HyprlandWindow::onWaylandSurfaceCreated() {
	auto* manager = impl::HyprlandSurfaceManager::instance();

	if (!manager->isActive()) {
		qWarning() << "The active compositor does not support the hyprland_surface_v1 protocol. "
		              "HyprlandWindow will not work.";
		return;
	}

	auto v = this->mWaylandWindow->property("hyprland_window_ext");
	if (v.canConvert<HyprlandWindow*>()) {
		auto* windowExt = v.value<HyprlandWindow*>();
		if (windowExt != this && windowExt->surface) {
			this->surface.swap(windowExt->surface);
		}
	}

	if (!this->surface) {
		auto* ext = manager->createHyprlandExtension(this->mWaylandWindow);
		this->surface = std::unique_ptr<impl::HyprlandSurface>(ext);
	}

	this->mWaylandWindow->setProperty("hyprland_window_ext", QVariant::fromValue(this));

	this->pendingPolish.opacity = this->mOpacity != 1.0;
	this->pendingPolish.visibleMask = this->mVisibleMask;

	if (this->pendingPolish.opacity || this->pendingPolish.visibleMask) {
		this->proxyWindow->schedulePolish();
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

	this->proxyWindow = nullptr;

	if (this->surface == nullptr) {
		this->deleteLater();
	}
}

} // namespace qs::hyprland::surface
