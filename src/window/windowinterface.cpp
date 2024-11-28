#include "windowinterface.hpp"

#include <qobject.h>
#include <qquickitem.h>

#include "proxywindow.hpp"

QsWindowAttached::QsWindowAttached(QQuickItem* parent): QObject(parent) {
	QObject::connect(parent, &QQuickItem::windowChanged, this, &QsWindowAttached::updateWindow);
}

QsWindowAttached* WindowInterface::qmlAttachedProperties(QObject* object) {
	while (object && !qobject_cast<QQuickItem*>(object)) {
		object = object->parent();
	}

	if (!object) return nullptr;
	auto* item = static_cast<QQuickItem*>(object); // NOLINT
	return new ProxyWindowAttached(item);
}
