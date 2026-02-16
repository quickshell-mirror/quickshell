#include "qml.hpp"
#include <memory>

#include <private/qwaylandwindow_p.h>
#include <qlogging.h>
#include <qobject.h>
#include <qregion.h>
#include <qtmetamacros.h>
#include <qvariant.h>
#include <qwindow.h>

#include "../../core/region.hpp"
#include "../../window/proxywindow.hpp"
#include "../../window/windowinterface.hpp"
#include "manager.hpp"
#include "surface.hpp"

using QtWaylandClient::QWaylandWindow;

namespace qs::wayland::background_effect {

BackgroundEffect* BackgroundEffect::qmlAttachedProperties(QObject* object) {
	auto* proxyWindow = qobject_cast<ProxyWindowBase*>(object);

	if (!proxyWindow) {
		if (auto* iface = qobject_cast<WindowInterface*>(object)) {
			proxyWindow = iface->proxyWindow();
		}
	}

	if (!proxyWindow) return nullptr;
	return new BackgroundEffect(proxyWindow);
}

BackgroundEffect::BackgroundEffect(ProxyWindowBase* window): QObject(nullptr), proxyWindow(window) {
	QObject::connect(
	    window,
	    &ProxyWindowBase::windowConnected,
	    this,
	    &BackgroundEffect::onWindowConnected
	);

	QObject::connect(window, &ProxyWindowBase::polished, this, &BackgroundEffect::onWindowPolished);

	QObject::connect(
	    window,
	    &ProxyWindowBase::devicePixelRatioChanged,
	    this,
	    &BackgroundEffect::updateBlurRegion
	);

	QObject::connect(window, &QObject::destroyed, this, &BackgroundEffect::onProxyWindowDestroyed);

	if (window->backingWindow()) {
		this->onWindowConnected();
	}
}

PendingRegion* BackgroundEffect::blurRegion() const { return this->mBlurRegion; }

void BackgroundEffect::setBlurRegion(PendingRegion* region) {
	if (region == this->mBlurRegion) return;

	if (this->mBlurRegion) {
		QObject::disconnect(this->mBlurRegion, nullptr, this, nullptr);
	}

	this->mBlurRegion = region;

	if (region) {
		QObject::connect(region, &QObject::destroyed, this, &BackgroundEffect::onBlurRegionDestroyed);
		QObject::connect(region, &PendingRegion::changed, this, &BackgroundEffect::updateBlurRegion);
	}

	this->updateBlurRegion();
	emit this->blurRegionChanged();
}

void BackgroundEffect::onBlurRegionDestroyed() {
	this->mBlurRegion = nullptr;
	this->updateBlurRegion();
	emit this->blurRegionChanged();
}

void BackgroundEffect::updateBlurRegion() {
	if (!this->surface || !this->proxyWindow) return;

	this->pendingBlurRegion = true;
	this->proxyWindow->schedulePolish();
}

void BackgroundEffect::onWindowPolished() {
	if (!this->surface || !this->pendingBlurRegion) return;

	QRegion region;
	if (this->mBlurRegion) {
		region =
		    this->mBlurRegion->applyTo(QRect(0, 0, this->mWindow->width(), this->mWindow->height()));

		auto margins = this->mWaylandWindow->clientSideMargins();
		region.translate(margins.left(), margins.top());
	}

	this->surface->setBlurRegion(region);
	this->pendingBlurRegion = false;
}

void BackgroundEffect::onWindowConnected() {
	this->mWindow = this->proxyWindow->backingWindow();

	QObject::connect(
	    this->mWindow,
	    &QWindow::visibleChanged,
	    this,
	    &BackgroundEffect::onWindowVisibleChanged
	);

	this->onWindowVisibleChanged();
}

void BackgroundEffect::onWindowVisibleChanged() {
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
	    &BackgroundEffect::onWaylandWindowDestroyed
	);

	QObject::connect(
	    this->mWaylandWindow,
	    &QWaylandWindow::surfaceCreated,
	    this,
	    &BackgroundEffect::onWaylandSurfaceCreated
	);

	QObject::connect(
	    this->mWaylandWindow,
	    &QWaylandWindow::surfaceDestroyed,
	    this,
	    &BackgroundEffect::onWaylandSurfaceDestroyed
	);

	if (this->mWaylandWindow->surface()) {
		this->onWaylandSurfaceCreated();
	}
}

void BackgroundEffect::onWaylandWindowDestroyed() { this->mWaylandWindow = nullptr; }

void BackgroundEffect::onWaylandSurfaceCreated() {
	auto* manager = impl::BackgroundEffectManager::instance();

	if (!manager) {
		qWarning() << "Cannot enable background effect as ext-background-effect-v1 is not supported "
		              "by the current compositor.";
		return;
	}

	// Steal protocol surface from previous BackgroundEffect to avoid duplicate-attachment on reload.
	auto v = this->mWaylandWindow->property("qs_background_effect");
	if (v.canConvert<BackgroundEffect*>()) {
		auto* prev = v.value<BackgroundEffect*>();
		if (prev != this && prev->surface) {
			this->surface.swap(prev->surface);
		}
	}

	if (!this->surface) {
		this->surface = std::unique_ptr<impl::BackgroundEffectSurface>(
		    manager->createEffectSurface(this->mWaylandWindow)
		);
	}

	this->mWaylandWindow->setProperty("qs_background_effect", QVariant::fromValue(this));

	this->pendingBlurRegion = this->mBlurRegion != nullptr;
	if (this->pendingBlurRegion) {
		this->proxyWindow->schedulePolish();
	}
}

void BackgroundEffect::onWaylandSurfaceDestroyed() {
	if (this->surface) this->surface->setInert();
	this->surface = nullptr;
	this->pendingBlurRegion = false;

	if (!this->proxyWindow) {
		this->deleteLater();
	}
}

void BackgroundEffect::onProxyWindowDestroyed() {
	// Don't delete the BackgroundEffect, and therefore the impl::BackgroundEffectSurface
	// until the wl_surface is destroyed. Deleting it when the proxy window is deleted would
	// cause a frame without blur between the destruction of the ext_background_effect_surface_v1
	// and wl_surface objects.

	this->proxyWindow = nullptr;

	if (this->surface == nullptr) {
		this->deleteLater();
	}
}

} // namespace qs::wayland::background_effect
