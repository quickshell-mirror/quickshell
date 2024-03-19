#include "floatingwindow.hpp"

#include <qobject.h>
#include <qqmlengine.h>
#include <qqmllist.h>
#include <qquickitem.h>
#include <qtypes.h>

#include "proxywindow.hpp"
#include "windowinterface.hpp"

void ProxyFloatingWindow::setWidth(qint32 width) {
	if (this->window == nullptr || !this->window->isVisible()) {
		this->ProxyWindowBase::setWidth(width);
	}
}

void ProxyFloatingWindow::setHeight(qint32 height) {
	if (this->window == nullptr || !this->window->isVisible()) {
		this->ProxyWindowBase::setHeight(height);
	}
}

// FloatingWindowInterface

FloatingWindowInterface::FloatingWindowInterface(QObject* parent)
    : WindowInterface(parent)
    , window(new ProxyFloatingWindow(this)) {
	// clang-format off
	QObject::connect(this->window, &ProxyWindowBase::windowConnected, this, &FloatingWindowInterface::windowConnected);
	QObject::connect(this->window, &ProxyWindowBase::visibleChanged, this, &FloatingWindowInterface::visibleChanged);
	QObject::connect(this->window, &ProxyWindowBase::heightChanged, this, &FloatingWindowInterface::heightChanged);
	QObject::connect(this->window, &ProxyWindowBase::widthChanged, this, &FloatingWindowInterface::widthChanged);
	QObject::connect(this->window, &ProxyWindowBase::screenChanged, this, &FloatingWindowInterface::screenChanged);
	QObject::connect(this->window, &ProxyWindowBase::colorChanged, this, &FloatingWindowInterface::colorChanged);
	QObject::connect(this->window, &ProxyWindowBase::maskChanged, this, &FloatingWindowInterface::maskChanged);
	// clang-format on
}

void FloatingWindowInterface::onReload(QObject* oldInstance) {
	QQmlEngine::setContextForObject(this->window, QQmlEngine::contextForObject(this));

	auto* old = qobject_cast<FloatingWindowInterface*>(oldInstance);
	this->window->onReload(old != nullptr ? old->window : nullptr);
}

QQmlListProperty<QObject> FloatingWindowInterface::data() { return this->window->data(); }
ProxyWindowBase* FloatingWindowInterface::proxyWindow() const { return this->window; }
QQuickItem* FloatingWindowInterface::contentItem() const { return this->window->contentItem(); }

// NOLINTBEGIN
#define proxyPair(type, get, set)                                                                  \
	type FloatingWindowInterface::get() const { return this->window->get(); }                        \
	void FloatingWindowInterface::set(type value) { this->window->set(value); }

proxyPair(bool, isVisible, setVisible);
proxyPair(qint32, width, setWidth);
proxyPair(qint32, height, setHeight);
proxyPair(QuickshellScreenInfo*, screen, setScreen);
proxyPair(QColor, color, setColor);
proxyPair(PendingRegion*, mask, setMask);

#undef proxyPair
// NOLINTEND
