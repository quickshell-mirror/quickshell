#include "windowinterface.hpp"

#include <qobject.h>
#include <qquickitem.h>
#include <qvariant.h>

#include "proxywindow.hpp"

QsWindowAttached* WindowInterface::qmlAttachedProperties(QObject* object) {
	auto* item = qobject_cast<QQuickItem*>(object);
	if (!item) return nullptr;
	auto* window = item->window();
	if (!window) return nullptr;
	auto* proxy = window->property("__qs_proxywindow").value<ProxyWindowBase*>();
	if (!proxy) return nullptr;

	auto v = proxy->property("__qs_window_attached");
	if (auto* attached = v.value<QsWindowAttached*>()) {
		return attached;
	}

	auto* attached = new ProxyWindowAttached(proxy);

	if (attached) {
		proxy->setProperty("__qs_window_attached", QVariant::fromValue(attached));
	}

	return attached;
}
