#include "inhibitor.hpp"

#include <private/qwaylandwindow_p.h>
#include <qlogging.h>
#include <qobject.h>
#include <qtmetamacros.h>

#include "../../window/proxywindow.hpp"
#include "../../window/windowinterface.hpp"
#include "proto.hpp"

namespace qs::wayland::shortcuts_inhibit {
using QtWaylandClient::QWaylandWindow;

ShortcutsInhibitor::ShortcutsInhibitor() {
	this->bBoundWindow.setBinding([this] {
		return this->bEnabled ? this->bWindowObject.value() : nullptr;
	});
}

ShortcutsInhibitor::~ShortcutsInhibitor() { delete this->inhibitor; }

QObject* ShortcutsInhibitor::window() const { return this->bWindowObject; }

void ShortcutsInhibitor::setWindow(QObject* window) {
	if (window == this->bWindowObject) return;

	auto* proxyWindow = qobject_cast<ProxyWindowBase*>(window);

	if (proxyWindow == nullptr) {
		if (auto* iface = qobject_cast<WindowInterface*>(window)) {
			proxyWindow = iface->proxyWindow();
		}
	}

	this->bWindowObject = proxyWindow ? window : nullptr;
}

void ShortcutsInhibitor::boundWindowChanged() {
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

		QObject::connect(
		    proxyWindow,
		    &QObject::destroyed,
		    this,
		    &ShortcutsInhibitor::onWindowDestroyed
		);

		QObject::connect(
		    proxyWindow,
		    &ProxyWindowBase::backerVisibilityChanged,
		    this,
		    &ShortcutsInhibitor::onWindowVisibilityChanged
		);

		this->onWindowVisibilityChanged();
	}

	emit this->windowChanged();
}

void ShortcutsInhibitor::onWindowDestroyed() {
	this->proxyWindow = nullptr;
	this->onWaylandSurfaceDestroyed();
	this->bWindowObject = nullptr;
}

void ShortcutsInhibitor::onWindowVisibilityChanged() {
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
	    &ShortcutsInhibitor::onWaylandWindowDestroyed
	);

	QObject::connect(
	    waylandWindow,
	    &QWaylandWindow::surfaceCreated,
	    this,
	    &ShortcutsInhibitor::onWaylandSurfaceCreated
	);

	QObject::connect(
	    waylandWindow,
	    &QWaylandWindow::surfaceDestroyed,
	    this,
	    &ShortcutsInhibitor::onWaylandSurfaceDestroyed
	);

	if (waylandWindow->surface()) this->onWaylandSurfaceCreated();
}

void ShortcutsInhibitor::onWaylandWindowDestroyed() { this->mWaylandWindow = nullptr; }

void ShortcutsInhibitor::onWaylandSurfaceCreated() {
	auto* manager = impl::ShortcutsInhibitManager::instance();

	if (!manager) {
		qWarning() << "Cannot enable shortcuts inhibitor as keyboard-shortcuts-inhibit-unstable-v1 is "
		              "not supported by "
		              "the current compositor.";
		return;
	}

	delete this->inhibitor;
	this->inhibitor = manager->createShortcutsInhibitor(this->mWaylandWindow);
}

void ShortcutsInhibitor::onWaylandSurfaceDestroyed() {
	delete this->inhibitor;
	this->inhibitor = nullptr;
}

} // namespace qs::wayland::shortcuts_inhibit