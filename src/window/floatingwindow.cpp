#include "floatingwindow.hpp"

#include <qobject.h>
#include <qqmlengine.h>
#include <qqmllist.h>
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

// FloatingWindowInterface

FloatingWindowInterface::FloatingWindowInterface(QObject* parent)
    : WindowInterface(parent)
    , window(new ProxyFloatingWindow(this)) {
	this->connectSignals();

	// clang-format off
	QObject::connect(this->window, &ProxyFloatingWindow::titleChanged, this, &FloatingWindowInterface::titleChanged);
	QObject::connect(this->window, &ProxyFloatingWindow::minimumSizeChanged, this, &FloatingWindowInterface::minimumSizeChanged);
	QObject::connect(this->window, &ProxyFloatingWindow::maximumSizeChanged, this, &FloatingWindowInterface::maximumSizeChanged);
	QObject::connect(this->window, &ProxyWindowBase::windowVisibilityChanged, this, &FloatingWindowInterface::onVisibilityChanged);
	// clang-format on
}

void FloatingWindowInterface::onReload(QObject* oldInstance) {
	QQmlEngine::setContextForObject(this->window, QQmlEngine::contextForObject(this));

	auto* old = qobject_cast<FloatingWindowInterface*>(oldInstance);
	this->window->reload(old != nullptr ? old->window : nullptr);
}

ProxyWindowBase* FloatingWindowInterface::proxyWindow() const { return this->window; }

void FloatingWindowInterface::onVisibilityChanged() {
	auto* qw = this->window->backingWindow();
	auto visibility = qw ? qw->visibility() : QWindow::Hidden;

	auto maximized = visibility == QWindow::Maximized;
	auto fullscreen = visibility == QWindow::FullScreen;

	if (maximized != this->mMaximized) {
		this->mMaximized = maximized;
		emit this->maximizedChanged();
	}

	if (fullscreen != this->mFullscreen) {
		this->mFullscreen = fullscreen;
		emit this->fullscreenChanged();
	}
}

bool FloatingWindowInterface::maximized() const { return this->mMaximized; }
bool FloatingWindowInterface::fullscreen() const { return this->mFullscreen; }

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

void FloatingWindowInterface::showNormal() const {
	if (auto* qw = this->window->backingWindow()) qw->showNormal();
}

void FloatingWindowInterface::showMaximized() const {
	if (auto* qw = this->window->backingWindow()) qw->showMaximized();
}

void FloatingWindowInterface::showMinimized() const {
	if (auto* qw = this->window->backingWindow()) qw->showMinimized();
}

void FloatingWindowInterface::showFullScreen() const {
	if (auto* qw = this->window->backingWindow()) qw->showFullScreen();
}
