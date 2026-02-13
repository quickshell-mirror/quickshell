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

BackgroundEffect::BackgroundEffect() {
	this->bBoundWindow.setBinding([this] {
		return this->bEnabled ? this->bWindowObject.value() : nullptr;
	});
}

BackgroundEffect::~BackgroundEffect() = default;

QObject* BackgroundEffect::window() const { return this->bWindowObject; }

void BackgroundEffect::setWindow(QObject* window) {
	if (window == this->bWindowObject) return;

	auto* proxyWindow = qobject_cast<ProxyWindowBase*>(window);

	if (!proxyWindow) {
		if (auto* iface = qobject_cast<WindowInterface*>(window)) {
			proxyWindow = iface->proxyWindow();
		}
	}

	this->bWindowObject = proxyWindow ? window : nullptr;
}

PendingRegion* BackgroundEffect::blurRegion() const { return this->mBlurRegion; }

void BackgroundEffect::setBlurRegion(PendingRegion* region) {
	if (region == this->mBlurRegion) return;

	if (this->mBlurRegion) {
		QObject::disconnect(this->mBlurRegion, nullptr, this, nullptr);
	}

	this->mBlurRegion = region;

	if (region) {
		QObject::connect(region, &QObject::destroyed, this, [this]() {
			this->mBlurRegion = nullptr;
			this->updateBlurRegion();
			emit this->blurRegionChanged();
		});

		QObject::connect(region, &PendingRegion::changed, this, &BackgroundEffect::updateBlurRegion);
	}

	this->updateBlurRegion();
	emit this->blurRegionChanged();
}

bool BackgroundEffect::active() const { return this->mActive; }

void BackgroundEffect::updateActive() {
	auto* manager = impl::BackgroundEffectManager::instance();
	auto active = this->surface != nullptr && manager && manager->blurAvailable();
	if (active == this->mActive) return;
	this->mActive = active;
	emit this->activeChanged();
}

void BackgroundEffect::updateBlurRegion() {
	if (!this->surface || !this->proxyWindow) return;
	this->pendingBlurRegion = true;
	this->proxyWindow->schedulePolish();
}

void BackgroundEffect::onWindowPolished() {
	if (!this->surface || !this->pendingBlurRegion) return;

	if (!this->mWaylandWindow || !this->mWaylandWindow->surface()) {
		this->pendingBlurRegion = false;
		return;
	}

	QRegion region;
	if (this->mBlurRegion) {
		region =
		    this->mBlurRegion->applyTo(QRect(0, 0, this->mWindow->width(), this->mWindow->height()));
	}

	this->surface->setBlurRegion(region);
	this->pendingBlurRegion = false;
}

void BackgroundEffect::onBoundWindowChanged() {
	auto* window = this->bBoundWindow.value();
	auto* proxyWindow = qobject_cast<ProxyWindowBase*>(window);

	if (!proxyWindow) {
		if (auto* iface = qobject_cast<WindowInterface*>(window)) {
			proxyWindow = iface->proxyWindow();
		}
	}

	if (proxyWindow == this->proxyWindow) return;

	if (this->mWaylandWindow) {
		QObject::disconnect(this->mWaylandWindow, nullptr, this, nullptr);
		this->mWaylandWindow = nullptr;

		// Don't use onWaylandSurfaceDestroyed - the wl_surface is still alive (not inert).
		this->surface = nullptr;
		this->pendingBlurRegion = false;

		auto* manager = impl::BackgroundEffectManager::instance();
		if (manager) QObject::disconnect(manager, nullptr, this, nullptr);

		this->updateActive();
	}

	if (this->proxyWindow) {
		QObject::disconnect(this->proxyWindow, nullptr, this, nullptr);
		this->proxyWindow = nullptr;
	}

	if (!proxyWindow) return;
	this->proxyWindow = proxyWindow;

	QObject::connect(proxyWindow, &QObject::destroyed, this, &BackgroundEffect::onWindowDestroyed);

	QObject::connect(
	    proxyWindow,
	    &ProxyWindowBase::backerVisibilityChanged,
	    this,
	    &BackgroundEffect::onWindowVisibilityChanged
	);

	QObject::connect(
	    proxyWindow,
	    &ProxyWindowBase::polished,
	    this,
	    &BackgroundEffect::onWindowPolished
	);

	this->onWindowVisibilityChanged();
}

void BackgroundEffect::onWindowDestroyed() {
	this->proxyWindow = nullptr;

	this->surface = nullptr;
	this->pendingBlurRegion = false;

	auto* manager = impl::BackgroundEffectManager::instance();
	if (manager) QObject::disconnect(manager, nullptr, this, nullptr);

	this->updateActive();
	this->bWindowObject = nullptr;
}

void BackgroundEffect::onWindowVisibilityChanged() {
	if (!this->proxyWindow->isVisibleDirect()) return;

	this->mWindow = this->proxyWindow->backingWindow();
	if (!this->mWindow->handle()) this->mWindow->create();

	auto* waylandWindow = dynamic_cast<QWaylandWindow*>(this->mWindow->handle());
	if (waylandWindow == this->mWaylandWindow) return;

	// Destroy protocol surface before disconnecting from old wayland window.
	this->surface = nullptr;
	this->pendingBlurRegion = false;
	this->updateActive();

	if (this->mWaylandWindow) {
		QObject::disconnect(this->mWaylandWindow, nullptr, this, nullptr);
	}

	this->mWaylandWindow = waylandWindow;
	if (!waylandWindow) return;

	QObject::connect(
	    waylandWindow,
	    &QObject::destroyed,
	    this,
	    &BackgroundEffect::onWaylandWindowDestroyed
	);

	QObject::connect(
	    waylandWindow,
	    &QWaylandWindow::surfaceCreated,
	    this,
	    &BackgroundEffect::onWaylandSurfaceCreated
	);

	QObject::connect(
	    waylandWindow,
	    &QWaylandWindow::surfaceDestroyed,
	    this,
	    &BackgroundEffect::onWaylandSurfaceDestroyed
	);

	if (waylandWindow->surface()) this->onWaylandSurfaceCreated();
}

void BackgroundEffect::onWaylandWindowDestroyed() { this->mWaylandWindow = nullptr; }

void BackgroundEffect::onWaylandSurfaceCreated() {
	auto* manager = impl::BackgroundEffectManager::instance();

	if (!manager) {
		qWarning() << "Cannot enable background effect as ext-background-effect-v1 is not supported "
		              "by the current compositor.";
		return;
	}

	QObject::connect(
	    manager,
	    &impl::BackgroundEffectManager::blurAvailableChanged,
	    this,
	    &BackgroundEffect::updateActive
	);

	// Steal protocol surface from previous BackgroundEffect to avoid duplicate-attachment on reload.
	auto v = this->mWaylandWindow->property("qs_background_effect");
	if (v.canConvert<BackgroundEffect*>()) {
		auto* prev = v.value<BackgroundEffect*>();
		if (prev != this && prev->surface) {
			this->surface.swap(prev->surface);
			prev->updateActive();
		}
	}

	if (!this->surface) {
		this->surface = std::unique_ptr<impl::BackgroundEffectSurface>(
		    manager->createEffectSurface(this->mWaylandWindow)
		);
	}

	this->mWaylandWindow->setProperty("qs_background_effect", QVariant::fromValue(this));
	this->updateActive();

	this->pendingBlurRegion = this->mBlurRegion != nullptr;
	if (this->pendingBlurRegion) {
		this->proxyWindow->schedulePolish();
	}
}

void BackgroundEffect::onWaylandSurfaceDestroyed() {
	if (this->surface) this->surface->setInert();
	this->surface = nullptr;
	this->pendingBlurRegion = false;

	auto* manager = impl::BackgroundEffectManager::instance();
	if (manager) QObject::disconnect(manager, nullptr, this, nullptr);

	this->updateActive();
}

} // namespace qs::wayland::background_effect
