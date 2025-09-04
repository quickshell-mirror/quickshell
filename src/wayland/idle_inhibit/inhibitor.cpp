#include "inhibitor.hpp"

#include <private/qwaylandwindow_p.h>
#include <qlogging.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include "../../window/proxywindow.hpp"
#include "../../window/windowinterface.hpp"
#include "proto.hpp"

namespace qs::wayland::idle_inhibit {
using QtWaylandClient::QWaylandWindow;

IdleInhibitor::IdleInhibitor() {
	this->bBoundWindow.setBinding([this] {
		return this->bEnabled ? this->bWindowObject.value() : nullptr;
	});
}

IdleInhibitor::~IdleInhibitor() { delete this->inhibitor; }

QObject* IdleInhibitor::window() const { return this->bWindowObject; }

void IdleInhibitor::setWindow(QObject* window) {
	if (window == this->bWindowObject) return;

	auto* proxyWindow = qobject_cast<ProxyWindowBase*>(window);

	if (proxyWindow == nullptr) {
		if (auto* iface = qobject_cast<WindowInterface*>(window)) {
			proxyWindow = iface->proxyWindow();
		}
	}

	this->bWindowObject = proxyWindow ? window : nullptr;
}

void IdleInhibitor::boundWindowChanged() {
	auto* window = this->bBoundWindow.value();
	auto* proxyWindow = qobject_cast<ProxyWindowBase*>(window);

	if (proxyWindow == nullptr) {
		if (auto* iface = qobject_cast<WindowInterface*>(window)) {
			proxyWindow = iface->proxyWindow();
		}
	}

	if (proxyWindow == this->proxyWindow) return;

	if (this->mWaylandWindow) {
		QObject::disconnect(this->mWaylandWindow, nullptr, this, nullptr);
		this->mWaylandWindow = nullptr;
		this->onWaylandSurfaceDestroyed();
	}

	if (this->proxyWindow) {
		QObject::disconnect(this->proxyWindow, nullptr, this, nullptr);
		this->proxyWindow = nullptr;
	}

	if (proxyWindow) {
		this->proxyWindow = proxyWindow;

		QObject::connect(proxyWindow, &QObject::destroyed, this, &IdleInhibitor::onWindowDestroyed);

		QObject::connect(
		    proxyWindow,
		    &ProxyWindowBase::backerVisibilityChanged,
		    this,
		    &IdleInhibitor::onWindowVisibilityChanged
		);

		this->onWindowVisibilityChanged();
	}

	emit this->windowChanged();
}

void IdleInhibitor::onWindowDestroyed() {
	this->proxyWindow = nullptr;
	this->onWaylandSurfaceDestroyed();
	this->bWindowObject = nullptr;
}

void IdleInhibitor::onWindowVisibilityChanged() {
	if (!this->proxyWindow->isVisibleDirect()) return;

	auto* window = this->proxyWindow->backingWindow();
	if (!window->handle()) window->create();

	auto* waylandWindow = dynamic_cast<QWaylandWindow*>(window->handle());
	if (waylandWindow == this->mWaylandWindow) return;
	this->mWaylandWindow = waylandWindow;

	QObject::connect(
	    waylandWindow,
	    &QObject::destroyed,
	    this,
	    &IdleInhibitor::onWaylandWindowDestroyed
	);

	QObject::connect(
	    waylandWindow,
	    &QWaylandWindow::surfaceCreated,
	    this,
	    &IdleInhibitor::onWaylandSurfaceCreated
	);

	QObject::connect(
	    waylandWindow,
	    &QWaylandWindow::surfaceDestroyed,
	    this,
	    &IdleInhibitor::onWaylandSurfaceDestroyed
	);

	if (waylandWindow->surface()) this->onWaylandSurfaceCreated();
}

void IdleInhibitor::onWaylandWindowDestroyed() { this->mWaylandWindow = nullptr; }

void IdleInhibitor::onWaylandSurfaceCreated() {
	auto* manager = impl::IdleInhibitManager::instance();

	if (!manager) {
		qWarning() << "Cannot enable idle inhibitor as idle-inhibit-unstable-v1 is not supported by "
		              "the current compositor.";
		return;
	}

	delete this->inhibitor;
	this->inhibitor = manager->createIdleInhibitor(this->mWaylandWindow);
}

void IdleInhibitor::onWaylandSurfaceDestroyed() {
	delete this->inhibitor;
	this->inhibitor = nullptr;
}

} // namespace qs::wayland::idle_inhibit
