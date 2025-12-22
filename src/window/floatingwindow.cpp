#include "floatingwindow.hpp"

#include <qnamespace.h>
#include <qobject.h>
#include <qqmlengine.h>
#include <qqmlinfo.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwindow.h>

#include "proxywindow.hpp"
#include "windowinterface.hpp"

void ProxyFloatingWindow::connectWindow() {
	this->ProxyWindowBase::connectWindow();

	this->window->setTitle(this->bTitle);
	this->window->setMinimumSize(this->bMinimumSize);
	this->window->setMaximumSize(this->bMaximumSize);
}

void ProxyFloatingWindow::setVisible(bool visible) {
	if (!visible) {
		QObject::disconnect(this->mParentVisibleConn);
		this->ProxyWindowBase::setVisible(false);
		return;
	}

	// If there's a parent, set transient before showing
	if (this->mParentProxyWindow) {
		auto* pw = this->mParentProxyWindow->backingWindow();
		if (pw && pw->isVisible()) {
			if (this->window) this->window->setTransientParent(pw);
		} else {
			// Parent not visible yet - wait for it.
			QObject::disconnect(this->mParentVisibleConn);
			this->mParentVisibleConn = QObject::connect(
			    this->mParentProxyWindow,
			    &ProxyWindowBase::backerVisibilityChanged,
			    this,
			    [this]() {
				    auto* pw =
				        this->mParentProxyWindow ? this->mParentProxyWindow->backingWindow() : nullptr;
				    if (!pw || !pw->isVisible() || !this->window) return;
				    this->window->setTransientParent(pw);
				    QObject::disconnect(this->mParentVisibleConn);
				    this->ProxyWindowBase::setVisible(true);
			    }
			);
			return;
		}
	}

	this->ProxyWindowBase::setVisible(true);
}

void ProxyFloatingWindow::trySetWidth(qint32 implicitWidth) {
	if (!this->window->isVisible()) {
		this->ProxyWindowBase::trySetWidth(implicitWidth);
	}
}

void ProxyFloatingWindow::trySetHeight(qint32 implicitHeight) {
	if (!this->window->isVisible()) {
		this->ProxyWindowBase::trySetHeight(implicitHeight);
	}
}

void ProxyFloatingWindow::onTitleChanged() {
	if (this->window) this->window->setTitle(this->bTitle);
	emit this->titleChanged();
}

void ProxyFloatingWindow::onMinimumSizeChanged() {
	if (this->window) this->window->setMinimumSize(this->bMinimumSize);
	emit this->minimumSizeChanged();
}

void ProxyFloatingWindow::onMaximumSizeChanged() {
	if (this->window) this->window->setMaximumSize(this->bMaximumSize);
	emit this->maximumSizeChanged();
}

QObject* ProxyFloatingWindow::parentWindow() const { return this->mParentWindow; }

void ProxyFloatingWindow::setParentWindow(QObject* window) {
	if (window == this->mParentWindow) return;

	if (this->window && this->window->isVisible()) {
		qmlWarning(this) << "parentWindow cannot be changed after the window is visible.";
		return;
	}

	QObject::disconnect(this->mParentVisibleConn);
	this->mParentWindow = nullptr;
	this->mParentProxyWindow = nullptr;

	if (window) {
		if (auto* proxy = qobject_cast<ProxyWindowBase*>(window)) {
			this->mParentProxyWindow = proxy;
		} else if (auto* iface = qobject_cast<WindowInterface*>(window)) {
			this->mParentProxyWindow = iface->proxyWindow();
		} else {
			qmlWarning(this) << "parentWindow must be a quickshell window.";
			return;
		}
		this->mParentWindow = window;
	}
}

// FloatingWindowInterface

FloatingWindowInterface::FloatingWindowInterface(QObject* parent)
    : WindowInterface(parent)
    , window(new ProxyFloatingWindow(this)) {
	this->connectSignals();

	// clang-format off
	QObject::connect(this->window, &ProxyFloatingWindow::titleChanged, this, &FloatingWindowInterface::titleChanged);
	QObject::connect(this->window, &ProxyFloatingWindow::minimumSizeChanged, this, &FloatingWindowInterface::minimumSizeChanged);
	QObject::connect(this->window, &ProxyFloatingWindow::maximumSizeChanged, this, &FloatingWindowInterface::maximumSizeChanged);
	QObject::connect(this->window, &ProxyWindowBase::windowConnected, this, &FloatingWindowInterface::onWindowConnected);
	// clang-format on
}

void FloatingWindowInterface::onReload(QObject* oldInstance) {
	QQmlEngine::setContextForObject(this->window, QQmlEngine::contextForObject(this));

	auto* old = qobject_cast<FloatingWindowInterface*>(oldInstance);
	this->window->reload(old != nullptr ? old->window : nullptr);
}

ProxyWindowBase* FloatingWindowInterface::proxyWindow() const { return this->window; }

void FloatingWindowInterface::onWindowConnected() {
	auto* qw = this->window->backingWindow();
	if (qw) {
		QObject::connect(
		    qw,
		    &QWindow::windowStateChanged,
		    this,
		    &FloatingWindowInterface::onWindowStateChanged
		);
		this->setMinimized(this->mMinimized);
		this->setMaximized(this->mMaximized);
		this->setFullscreen(this->mFullscreen);
		this->onWindowStateChanged();
	}
}

void FloatingWindowInterface::onWindowStateChanged() {
	auto* qw = this->window->backingWindow();
	auto states = qw ? qw->windowStates() : Qt::WindowStates();

	auto minimized = states.testFlag(Qt::WindowMinimized);
	auto maximized = states.testFlag(Qt::WindowMaximized);
	auto fullscreen = states.testFlag(Qt::WindowFullScreen);

	if (minimized != this->mWasMinimized) {
		this->mWasMinimized = minimized;
		emit this->minimizedChanged();
	}

	if (maximized != this->mWasMaximized) {
		this->mWasMaximized = maximized;
		emit this->maximizedChanged();
	}

	if (fullscreen != this->mWasFullscreen) {
		this->mWasFullscreen = fullscreen;
		emit this->fullscreenChanged();
	}
}

bool FloatingWindowInterface::isMinimized() const {
	auto* qw = this->window->backingWindow();
	if (!qw) return this->mWasMinimized;
	return qw->windowStates().testFlag(Qt::WindowMinimized);
}

void FloatingWindowInterface::setMinimized(bool minimized) {
	this->mMinimized = minimized;

	if (auto* qw = this->window->backingWindow()) {
		auto states = qw->windowStates();
		states.setFlag(Qt::WindowMinimized, minimized);
		qw->setWindowStates(states);
	}
}

bool FloatingWindowInterface::isMaximized() const {
	auto* qw = this->window->backingWindow();
	if (!qw) return this->mWasMaximized;
	return qw->windowStates().testFlag(Qt::WindowMaximized);
}

void FloatingWindowInterface::setMaximized(bool maximized) {
	this->mMaximized = maximized;

	if (auto* qw = this->window->backingWindow()) {
		auto states = qw->windowStates();
		states.setFlag(Qt::WindowMaximized, maximized);
		qw->setWindowStates(states);
	}
}

bool FloatingWindowInterface::isFullscreen() const {
	auto* qw = this->window->backingWindow();
	if (!qw) return this->mWasFullscreen;
	return qw->windowStates().testFlag(Qt::WindowFullScreen);
}

void FloatingWindowInterface::setFullscreen(bool fullscreen) {
	this->mFullscreen = fullscreen;

	if (auto* qw = this->window->backingWindow()) {
		auto states = qw->windowStates();
		states.setFlag(Qt::WindowFullScreen, fullscreen);
		qw->setWindowStates(states);
	}
}

bool FloatingWindowInterface::startSystemMove() const {
	auto* qw = this->window->backingWindow();
	if (!qw) return false;
	return qw->startSystemMove();
}

bool FloatingWindowInterface::startSystemResize(Qt::Edges edges) const {
	auto* qw = this->window->backingWindow();
	if (!qw) return false;
	return qw->startSystemResize(edges);
}

QObject* FloatingWindowInterface::parentWindow() const { return this->window->parentWindow(); }

void FloatingWindowInterface::setParentWindow(QObject* window) {
	this->window->setParentWindow(window);
}
