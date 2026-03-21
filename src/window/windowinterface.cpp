#include "windowinterface.hpp"

#include <qlogging.h>
#include <qobject.h>
#include <qqmllist.h>
#include <qquickitem.h>
#include <qtypes.h>

#include "../core/qmlscreen.hpp"
#include "../core/region.hpp"
#include "proxywindow.hpp"

QPointF WindowInterface::itemPosition(QQuickItem* item) const {
	return this->proxyWindow()->itemPosition(item);
}

QRectF WindowInterface::itemRect(QQuickItem* item) const {
	return this->proxyWindow()->itemRect(item);
}

QPointF WindowInterface::mapFromItem(QQuickItem* item, QPointF point) const {
	return this->proxyWindow()->mapFromItem(item, point);
}

QPointF WindowInterface::mapFromItem(QQuickItem* item, qreal x, qreal y) const {
	return this->proxyWindow()->mapFromItem(item, x, y);
}

QRectF WindowInterface::mapFromItem(QQuickItem* item, QRectF rect) const {
	return this->proxyWindow()->mapFromItem(item, rect);
}

QRectF
WindowInterface::mapFromItem(QQuickItem* item, qreal x, qreal y, qreal width, qreal height) const {
	return this->proxyWindow()->mapFromItem(item, x, y, width, height);
}

QsWindowAttached::QsWindowAttached(QQuickItem* parent): QObject(parent) {
	QObject::connect(parent, &QQuickItem::windowChanged, this, &QsWindowAttached::updateWindow);
}

QPointF QsWindowAttached::itemPosition(QQuickItem* item) const {
	if (auto* proxyWindow = this->proxyWindow()) {
		return proxyWindow->itemPosition(item);
	} else {
		qCritical() << "Cannot call itemPosition before item is a member of a window.";
		return {};
	}
}

QRectF QsWindowAttached::itemRect(QQuickItem* item) const {
	if (auto* proxyWindow = this->proxyWindow()) {
		return proxyWindow->itemRect(item);
	} else {
		qCritical() << "Cannot call itemRect before item is a member of a window.";
		return {};
	}
}

QPointF QsWindowAttached::mapFromItem(QQuickItem* item, QPointF point) const {
	if (auto* proxyWindow = this->proxyWindow()) {
		return proxyWindow->mapFromItem(item, point);
	} else {
		qCritical() << "Cannot call mapFromItem before item is a member of a window.";
		return {};
	}
}

QPointF QsWindowAttached::mapFromItem(QQuickItem* item, qreal x, qreal y) const {
	if (auto* proxyWindow = this->proxyWindow()) {
		return proxyWindow->mapFromItem(item, x, y);
	} else {
		qCritical() << "Cannot call mapFromItem before item is a member of a window.";
		return {};
	}
}

QRectF QsWindowAttached::mapFromItem(QQuickItem* item, QRectF rect) const {
	if (auto* proxyWindow = this->proxyWindow()) {
		return proxyWindow->mapFromItem(item, rect);
	} else {
		qCritical() << "Cannot call mapFromItem before item is a member of a window.";
		return {};
	}
}

QRectF
QsWindowAttached::mapFromItem(QQuickItem* item, qreal x, qreal y, qreal width, qreal height) const {
	if (auto* proxyWindow = this->proxyWindow()) {
		return proxyWindow->mapFromItem(item, x, y, width, height);
	} else {
		qCritical() << "Cannot call mapFromItem before item is a member of a window.";
		return {};
	}
}

// clang-format off
QQuickItem* WindowInterface::contentItem() const { return this->proxyWindow()->contentItem(); }

bool WindowInterface::isVisible() const { return this->proxyWindow()->isVisible(); };
bool WindowInterface::isBackingWindowVisible() const { return this->proxyWindow()->isVisibleDirect(); };
void WindowInterface::setVisible(bool visible) const { this->proxyWindow()->setVisible(visible); };

qint32 WindowInterface::implicitWidth() const { return this->proxyWindow()->implicitWidth(); };
void WindowInterface::setImplicitWidth(qint32 implicitWidth) const { this->proxyWindow()->setImplicitWidth(implicitWidth); };

qint32 WindowInterface::implicitHeight() const { return this->proxyWindow()->implicitHeight(); };
void WindowInterface::setImplicitHeight(qint32 implicitHeight) const { this->proxyWindow()->setImplicitHeight(implicitHeight); };

qint32 WindowInterface::width() const { return this->proxyWindow()->width(); };
void WindowInterface::setWidth(qint32 width) const { this->proxyWindow()->setWidth(width); };

qint32 WindowInterface::height() const { return this->proxyWindow()->height(); };
void WindowInterface::setHeight(qint32 height) const { this->proxyWindow()->setHeight(height); };

qreal WindowInterface::devicePixelRatio() const { return this->proxyWindow()->devicePixelRatio(); };

QuickshellScreenInfo* WindowInterface::screen() const { return this->proxyWindow()->screen(); };
void WindowInterface::setScreen(QuickshellScreenInfo* screen) const { this->proxyWindow()->setScreen(screen); };

QColor WindowInterface::color() const { return this->proxyWindow()->color(); };
void WindowInterface::setColor(QColor color) const { this->proxyWindow()->setColor(color); };

PendingRegion* WindowInterface::mask() const { return this->proxyWindow()->mask(); };
void WindowInterface::setMask(PendingRegion* mask) const { this->proxyWindow()->setMask(mask); };

QsSurfaceFormat WindowInterface::surfaceFormat() const { return this->proxyWindow()->surfaceFormat(); };
void WindowInterface::setSurfaceFormat(QsSurfaceFormat format) const { this->proxyWindow()->setSurfaceFormat(format); };

bool WindowInterface::updatesEnabled() const { return this->proxyWindow()->updatesEnabled(); };
void WindowInterface::setUpdatesEnabled(bool updatesEnabled) const { this->proxyWindow()->setUpdatesEnabled(updatesEnabled); };

QQmlListProperty<QObject> WindowInterface::data() const { return this->proxyWindow()->data(); };
// clang-format on

void WindowInterface::connectSignals() const {
	auto* window = this->proxyWindow();
	// clang-format off
	QObject::connect(window, &ProxyWindowBase::closed, this, &WindowInterface::closed);
	QObject::connect(window, &ProxyWindowBase::resourcesLost, this, &WindowInterface::resourcesLost);
	QObject::connect(window, &ProxyWindowBase::windowConnected, this, &WindowInterface::windowConnected);
	QObject::connect(window, &ProxyWindowBase::visibleChanged, this, &WindowInterface::visibleChanged);
	QObject::connect(window, &ProxyWindowBase::backerVisibilityChanged, this, &WindowInterface::backingWindowVisibleChanged);
	QObject::connect(window, &ProxyWindowBase::implicitHeightChanged, this, &WindowInterface::implicitHeightChanged);
	QObject::connect(window, &ProxyWindowBase::implicitWidthChanged, this, &WindowInterface::implicitWidthChanged);
	QObject::connect(window, &ProxyWindowBase::heightChanged, this, &WindowInterface::heightChanged);
	QObject::connect(window, &ProxyWindowBase::widthChanged, this, &WindowInterface::widthChanged);
	QObject::connect(window, &ProxyWindowBase::devicePixelRatioChanged, this, &WindowInterface::devicePixelRatioChanged);
	QObject::connect(window, &ProxyWindowBase::screenChanged, this, &WindowInterface::screenChanged);
	QObject::connect(window, &ProxyWindowBase::windowTransformChanged, this, &WindowInterface::windowTransformChanged);
	QObject::connect(window, &ProxyWindowBase::colorChanged, this, &WindowInterface::colorChanged);
	QObject::connect(window, &ProxyWindowBase::maskChanged, this, &WindowInterface::maskChanged);
	QObject::connect(window, &ProxyWindowBase::surfaceFormatChanged, this, &WindowInterface::surfaceFormatChanged);
	QObject::connect(window, &ProxyWindowBase::updatesEnabledChanged, this, &WindowInterface::updatesEnabledChanged);
	// clang-format on
}

QsWindowAttached* WindowInterface::qmlAttachedProperties(QObject* object) {
	while (object && !qobject_cast<QQuickItem*>(object)) {
		object = object->parent();
	}

	if (!object) return nullptr;
	auto* item = static_cast<QQuickItem*>(object); // NOLINT
	return new ProxyWindowAttached(item);
}
