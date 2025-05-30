#include "windowinterface.hpp"

#include <qlogging.h>
#include <qobject.h>
#include <qquickitem.h>
#include <qtypes.h>

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

QsWindowAttached* WindowInterface::qmlAttachedProperties(QObject* object) {
	while (object && !qobject_cast<QQuickItem*>(object)) {
		object = object->parent();
	}

	if (!object) return nullptr;
	auto* item = static_cast<QQuickItem*>(object); // NOLINT
	return new ProxyWindowAttached(item);
}
