#include "wayland_window.hpp"

#include <private/qwaylandshellsurface_p.h>
#include <private/qwaylandwindow_p.h>
#include <private/wayland-xdg-shell-client-protocol.h>
#include <qguiapplication.h>
#include <qobject.h>
#include <qqmlinfo.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtversionchecks.h>
#include <qwindow.h>

#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
#include <qnamespace.h>
#endif

#include "../window/proxywindow.hpp"

using QtWaylandClient::QWaylandWindow;

namespace qs::wayland {

WaylandWindow* WaylandWindow::qmlAttachedProperties(QObject* object) {
	auto* proxyWindow = ProxyWindowBase::forObject(object);

	if (!proxyWindow) return nullptr;
	return new WaylandWindow(proxyWindow);
}

WaylandWindow::WaylandWindow(ProxyWindowBase* window): QObject(window), proxyWindow(window) {
	QObject::connect(
	    window,
	    &ProxyWindowBase::windowConnected,
	    this,
	    &WaylandWindow::onWindowConnected
	);

	if (window->backingWindow()) {
		this->onWindowConnected();
	}
}

QString WaylandWindow::appId() const { return this->mAppId; }

void WaylandWindow::setAppId(const QString& appId) {
	if (appId == this->mAppId) return;
	this->mAppId = appId;
	this->applyAppId();
	emit this->appIdChanged();
}

void WaylandWindow::onWindowConnected() {
	this->mWindow = this->proxyWindow->backingWindow();

	QObject::connect(
	    this->mWindow,
	    &QWindow::visibleChanged,
	    this,
	    &WaylandWindow::onWindowVisibleChanged
	);

	this->onWindowVisibleChanged();
}

void WaylandWindow::onWindowVisibleChanged() {
	if (this->mWindow->isVisible() && !this->mWindow->handle()) {
		this->mWindow->create();
	}

#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
	// Qt < 6.8 has no surfaceRoleCreated signal. The shell surface is created and the
	// default app id set after visibleChanged is emitted, so apply ours in a queued
	// slot which runs after setVisible completes and before the toplevel is mapped.
	if (this->mWindow->isVisible()) {
		QMetaObject::invokeMethod(this, &WaylandWindow::onSurfaceRoleCreated, Qt::QueuedConnection);
	}
#endif

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
	    &WaylandWindow::onWaylandWindowDestroyed
	);

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
	QObject::connect(
	    this->mWaylandWindow,
	    &QWaylandWindow::surfaceRoleCreated,
	    this,
	    &WaylandWindow::onSurfaceRoleCreated
	);
#endif

	if (this->mWaylandWindow->shellSurface() && !this->mAppId.isEmpty()) {
		this->applyAppId();
	}
}

void WaylandWindow::onWaylandWindowDestroyed() { this->mWaylandWindow = nullptr; }

void WaylandWindow::onSurfaceRoleCreated() {
	if (this->mAppId.isEmpty()) return;
	this->applyAppId();
}

void WaylandWindow::applyAppId() {
	if (!this->mWaylandWindow) return;

	auto* shellSurface = this->mWaylandWindow->shellSurface();
	if (!shellSurface) return;

	if (!this->mWaylandWindow->surfaceRole<::xdg_toplevel>()) {
		qmlWarning(this) << "appId can only be set on toplevel windows.";
		return;
	}

	auto appId = this->mAppId.isEmpty() ? QGuiApplication::desktopFileName() : this->mAppId;
	shellSurface->setAppId(appId);
}

} // namespace qs::wayland
