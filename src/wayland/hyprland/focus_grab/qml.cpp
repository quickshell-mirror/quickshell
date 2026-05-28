#include "qml.hpp"
#include <utility>

#include <qlist.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qwindow.h>

#include "../../../window/proxywindow.hpp"
#include "grab.hpp"
#include "manager.hpp"

namespace qs::hyprland {
using focus_grab::FocusGrab;
using focus_grab::FocusGrabManager;

void HyprlandFocusGrab::componentComplete() { this->tryActivate(); }

bool HyprlandFocusGrab::isActive() const { return this->grab != nullptr && this->grab->isActive(); }

void HyprlandFocusGrab::setActive(bool active) {
	if (active == this->targetActive) return;
	this->targetActive = active;

	if (!active) {
		delete this->grab;
		this->grab = nullptr;
		emit this->activeChanged();
	} else {
		this->tryActivate();
	}
}

QObjectList HyprlandFocusGrab::windows() const { return this->windowObjects; }

void HyprlandFocusGrab::setWindows(QObjectList windows) {
	if (windows == this->windowObjects) return;
	if (this->grab) this->grab->startTransaction();

	for (auto* obj: this->windowObjects) {
		if (windows.contains(obj)) continue;
		QObject::disconnect(obj, nullptr, this, nullptr);

		auto* proxy = ProxyWindowBase::forObject(obj);
		if (!proxy) continue;

		QObject::disconnect(proxy, nullptr, this, nullptr);

		if (this->grab && proxy->backingWindow()) {
			this->grab->removeWindow(proxy->backingWindow());
		}
	}

	for (auto it = windows.begin(); it != windows.end();) {
		auto* proxy = ProxyWindowBase::forObject(*it);
		if (!proxy) {
			it = windows.erase(it);
			continue;
		}

		if (this->windowObjects.contains(*it)) {
			++it;
			continue;
		}

		QObject::connect(*it, &QObject::destroyed, this, &HyprlandFocusGrab::onObjectDestroyed);
		QObject::connect(
		    proxy,
		    &ProxyWindowBase::windowConnected,
		    this,
		    &HyprlandFocusGrab::onProxyConnected
		);

		if (this->grab && proxy->backingWindow()) {
			this->grab->addWindow(proxy->backingWindow());
		}

		++it;
	}

	if (this->grab) this->grab->completeTransaction();
	this->windowObjects = std::move(windows);
	emit this->windowsChanged();
}

void HyprlandFocusGrab::onGrabActivated() { emit this->activeChanged(); }

void HyprlandFocusGrab::onGrabCleared() {
	emit this->cleared();
	this->setActive(false);
}

void HyprlandFocusGrab::onProxyConnected() {
	if (this->grab != nullptr) {
		this->grab->addWindow(qobject_cast<ProxyWindowBase*>(this->sender())->backingWindow());
		this->grab->sync();
	}
}

void HyprlandFocusGrab::tryActivate() {
	if (!this->targetActive || this->grab) return;

	auto* manager = FocusGrabManager::instance();
	if (!manager->isActive()) {
		qWarning() << "The active compositor does not support the hyprland_focus_grab_v1 protocol. "
		              "HyprlandFocusGrab will not work.";
		qWarning() << "** Learn why $XDG_CURRENT_DESKTOP sucks and download a better compositor "
		              "today at https://hyprland.org";
		return;
	}

	this->grab = manager->createGrab();
	this->grab->setParent(this);
	QObject::connect(this->grab, &FocusGrab::activated, this, &HyprlandFocusGrab::onGrabActivated);
	QObject::connect(this->grab, &FocusGrab::cleared, this, &HyprlandFocusGrab::onGrabCleared);

	this->grab->startTransaction();
	for (auto* obj: this->windowObjects) {
		auto* proxy = ProxyWindowBase::forObject(obj);
		if (proxy && proxy->backingWindow()) {
			this->grab->addWindow(proxy->backingWindow());
		}
	}
	this->grab->completeTransaction();
}

void HyprlandFocusGrab::onObjectDestroyed(QObject* object) {
	this->windowObjects.removeOne(object);
	emit this->windowsChanged();
}

} // namespace qs::hyprland
