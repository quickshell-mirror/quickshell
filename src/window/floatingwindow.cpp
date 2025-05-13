#include "floatingwindow.hpp"

#include <qobject.h>
#include <qqmlengine.h>
#include <qqmllist.h>
#include <qquickitem.h>
#include <qtypes.h>

#include "proxywindow.hpp"
#include "windowinterface.hpp"

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

// FloatingWindowInterface

FloatingWindowInterface::FloatingWindowInterface(QObject* parent)
    : WindowInterface(parent)
    , window(new ProxyFloatingWindow(this)) {
	// clang-format off
	QObject::connect(this->window, &ProxyWindowBase::windowConnected, this, &FloatingWindowInterface::windowConnected);
	QObject::connect(this->window, &ProxyWindowBase::visibleChanged, this, &FloatingWindowInterface::visibleChanged);
	QObject::connect(this->window, &ProxyWindowBase::backerVisibilityChanged, this, &FloatingWindowInterface::backingWindowVisibleChanged);
	QObject::connect(this->window, &ProxyWindowBase::heightChanged, this, &FloatingWindowInterface::heightChanged);
	QObject::connect(this->window, &ProxyWindowBase::widthChanged, this, &FloatingWindowInterface::widthChanged);
	QObject::connect(this->window, &ProxyWindowBase::implicitHeightChanged, this, &FloatingWindowInterface::implicitHeightChanged);
	QObject::connect(this->window, &ProxyWindowBase::implicitWidthChanged, this, &FloatingWindowInterface::implicitWidthChanged);
	QObject::connect(this->window, &ProxyWindowBase::devicePixelRatioChanged, this, &FloatingWindowInterface::devicePixelRatioChanged);
	QObject::connect(this->window, &ProxyWindowBase::screenChanged, this, &FloatingWindowInterface::screenChanged);
	QObject::connect(this->window, &ProxyWindowBase::windowTransformChanged, this, &FloatingWindowInterface::windowTransformChanged);
	QObject::connect(this->window, &ProxyWindowBase::colorChanged, this, &FloatingWindowInterface::colorChanged);
	QObject::connect(this->window, &ProxyWindowBase::maskChanged, this, &FloatingWindowInterface::maskChanged);
	QObject::connect(this->window, &ProxyWindowBase::surfaceFormatChanged, this, &FloatingWindowInterface::surfaceFormatChanged);
	// clang-format on
}

void FloatingWindowInterface::onReload(QObject* oldInstance) {
	QQmlEngine::setContextForObject(this->window, QQmlEngine::contextForObject(this));

	auto* old = qobject_cast<FloatingWindowInterface*>(oldInstance);
	this->window->reload(old != nullptr ? old->window : nullptr);
}

QQmlListProperty<QObject> FloatingWindowInterface::data() { return this->window->data(); }
ProxyWindowBase* FloatingWindowInterface::proxyWindow() const { return this->window; }
QQuickItem* FloatingWindowInterface::contentItem() const { return this->window->contentItem(); }

bool FloatingWindowInterface::isBackingWindowVisible() const {
	return this->window->isVisibleDirect();
}

qreal FloatingWindowInterface::devicePixelRatio() const { return this->window->devicePixelRatio(); }

// NOLINTBEGIN
#define proxyPair(type, get, set)                                                                  \
	type FloatingWindowInterface::get() const { return this->window->get(); }                        \
	void FloatingWindowInterface::set(type value) { this->window->set(value); }

proxyPair(bool, isVisible, setVisible);
proxyPair(qint32, implicitWidth, setImplicitWidth);
proxyPair(qint32, implicitHeight, setImplicitHeight);
proxyPair(qint32, width, setWidth);
proxyPair(qint32, height, setHeight);
proxyPair(QuickshellScreenInfo*, screen, setScreen);
proxyPair(QColor, color, setColor);
proxyPair(PendingRegion*, mask, setMask);
proxyPair(QsSurfaceFormat, surfaceFormat, setSurfaceFormat);

#undef proxyPair
// NOLINTEND
