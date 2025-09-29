#include "inhibitor.hpp"

#include <private/qwaylandwindow_p.h>
#include <qguiapplication.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qwindow.h>

#include "../../window/proxywindow.hpp"
#include "../../window/windowinterface.hpp"
#include "proto.hpp"

namespace qs::wayland::shortcuts_inhibit {
using QtWaylandClient::QWaylandWindow;

ShortcutInhibitor::ShortcutInhibitor() {
	this->bBoundWindow.setBinding([this] {
		return this->bEnabled ? this->bWindowObject.value() : nullptr;
	});

	this->bActive.setBinding([this]() {
		auto* inhibitor = this->bInhibitor.value();
		if (!inhibitor) return false;
		if (!inhibitor->bindableActive().value()) return false;
		return this->bWindow.value() == this->bFocusedWindow;
	});

	QObject::connect(
	    dynamic_cast<QGuiApplication*>(QGuiApplication::instance()),
	    &QGuiApplication::focusWindowChanged,
	    this,
	    &ShortcutInhibitor::onFocusedWindowChanged
	);

	this->onFocusedWindowChanged(QGuiApplication::focusWindow());
}

ShortcutInhibitor::~ShortcutInhibitor() {
	if (!this->bInhibitor) return;

	auto* manager = impl::ShortcutsInhibitManager::instance();
	if (!manager) return;

	manager->unrefShortcutsInhibitor(this->bInhibitor);
}

void ShortcutInhibitor::onBoundWindowChanged() {
	auto* window = this->bBoundWindow.value();
	auto* proxyWindow = qobject_cast<ProxyWindowBase*>(window);

	if (!proxyWindow) {
		if (auto* iface = qobject_cast<WindowInterface*>(window)) {
			proxyWindow = iface->proxyWindow();
		}
	}

	if (proxyWindow == this->proxyWindow) return;

	if (this->proxyWindow) {
		QObject::disconnect(this->proxyWindow, nullptr, this, nullptr);
		this->proxyWindow = nullptr;
	}

	if (this->mWaylandWindow) {
		QObject::disconnect(this->mWaylandWindow, nullptr, this, nullptr);
		this->mWaylandWindow = nullptr;
		this->onWaylandSurfaceDestroyed();
	}

	if (proxyWindow) {
		this->proxyWindow = proxyWindow;

		QObject::connect(proxyWindow, &QObject::destroyed, this, &ShortcutInhibitor::onWindowDestroyed);

		QObject::connect(
		    proxyWindow,
		    &ProxyWindowBase::backerVisibilityChanged,
		    this,
		    &ShortcutInhibitor::onWindowVisibilityChanged
		);

		this->onWindowVisibilityChanged();
	}
}

void ShortcutInhibitor::onWindowDestroyed() {
	this->proxyWindow = nullptr;
	this->onWaylandSurfaceDestroyed();
}

void ShortcutInhibitor::onWindowVisibilityChanged() {
	if (!this->proxyWindow->isVisibleDirect()) return;

	auto* window = this->proxyWindow->backingWindow();
	if (!window->handle()) window->create();

	auto* waylandWindow = dynamic_cast<QWaylandWindow*>(window->handle());
	if (!waylandWindow) {
		qCCritical(impl::logShortcutsInhibit()) << "Window handle is not a QWaylandWindow";
		return;
	}
	if (waylandWindow == this->mWaylandWindow) return;
	this->mWaylandWindow = waylandWindow;
	this->bWindow = window;

	QObject::connect(
	    waylandWindow,
	    &QObject::destroyed,
	    this,
	    &ShortcutInhibitor::onWaylandWindowDestroyed
	);

	QObject::connect(
	    waylandWindow,
	    &QWaylandWindow::surfaceCreated,
	    this,
	    &ShortcutInhibitor::onWaylandSurfaceCreated
	);

	QObject::connect(
	    waylandWindow,
	    &QWaylandWindow::surfaceDestroyed,
	    this,
	    &ShortcutInhibitor::onWaylandSurfaceDestroyed
	);

	if (waylandWindow->surface()) this->onWaylandSurfaceCreated();
}

void ShortcutInhibitor::onWaylandWindowDestroyed() { this->mWaylandWindow = nullptr; }

void ShortcutInhibitor::onWaylandSurfaceCreated() {
	auto* manager = impl::ShortcutsInhibitManager::instance();

	if (!manager) {
		qWarning() << "Cannot enable shortcuts inhibitor as keyboard-shortcuts-inhibit-unstable-v1 is "
		              "not supported by "
		              "the current compositor.";
		return;
	}

	if (this->bInhibitor) {
		qFatal("ShortcutsInhibitor: inhibitor already exists when creating surface");
	}

	this->bInhibitor = manager->createShortcutsInhibitor(this->mWaylandWindow);
}

void ShortcutInhibitor::onWaylandSurfaceDestroyed() {
	if (!this->bInhibitor) return;

	auto* manager = impl::ShortcutsInhibitManager::instance();
	if (!manager) return;

	manager->unrefShortcutsInhibitor(this->bInhibitor);
	this->bInhibitor = nullptr;
}

void ShortcutInhibitor::onInhibitorChanged() {
	auto* inhibitor = this->bInhibitor.value();
	if (inhibitor) {
		QObject::connect(
		    inhibitor,
		    &impl::ShortcutsInhibitor::activeChanged,
		    this,
		    &ShortcutInhibitor::onInhibitorActiveChanged
		);
	}
}

void ShortcutInhibitor::onInhibitorActiveChanged() {
	auto* inhibitor = this->bInhibitor.value();
	if (inhibitor && !inhibitor->isActive()) {
		// Compositor has deactivated the inhibitor, making it invalid.
		// Set enabled to false so the user can enable it again to create a new inhibitor.
		this->bEnabled = false;
		emit this->cancelled();
	}
}

void ShortcutInhibitor::onFocusedWindowChanged(QWindow* focusedWindow) {
	this->bFocusedWindow = focusedWindow;
}

} // namespace qs::wayland::shortcuts_inhibit
