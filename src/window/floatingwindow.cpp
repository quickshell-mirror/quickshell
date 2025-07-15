#include "floatingwindow.hpp"

#include <qobject.h>
#include <qqmlengine.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

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
	// clang-format on
}

void FloatingWindowInterface::onReload(QObject* oldInstance) {
	QQmlEngine::setContextForObject(this->window, QQmlEngine::contextForObject(this));

	auto* old = qobject_cast<FloatingWindowInterface*>(oldInstance);
	this->window->reload(old != nullptr ? old->window : nullptr);
}

ProxyWindowBase* FloatingWindowInterface::proxyWindow() const { return this->window; }
