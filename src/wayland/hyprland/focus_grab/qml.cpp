#include "qml.hpp"
#include <utility>

#include <qlist.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qwindow.h>

#include "../../../window/proxywindow.hpp"
#include "../../../window/windowinterface.hpp"
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
	this->windowObjects = std::move(windows);
	this->syncWindows();
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
	for (auto* proxy: this->trackedProxies) {
		if (proxy->backingWindow() != nullptr) {
			this->grab->addWindow(proxy->backingWindow());
		}
	}
	this->grab->completeTransaction();
}

void HyprlandFocusGrab::syncWindows() {
	auto newProxy = QList<ProxyWindowBase*>();
	for (auto* windowObject: this->windowObjects) {
		auto* proxyWindow = qobject_cast<ProxyWindowBase*>(windowObject);

		if (proxyWindow == nullptr) {
			if (auto* iface = qobject_cast<WindowInterface*>(windowObject)) {
				proxyWindow = iface->proxyWindow();
			}
		}

		if (proxyWindow != nullptr) {
			newProxy.push_back(proxyWindow);
		}
	}

	if (this->grab) this->grab->startTransaction();

	for (auto* oldWindow: this->trackedProxies) {
		if (!newProxy.contains(oldWindow)) {
			QObject::disconnect(oldWindow, nullptr, this, nullptr);

			if (this->grab != nullptr && oldWindow->backingWindow() != nullptr) {
				this->grab->removeWindow(oldWindow->backingWindow());
			}
		}
	}

	for (auto* newProxy: newProxy) {
		if (!this->trackedProxies.contains(newProxy)) {
			QObject::connect(
			    newProxy,
			    &ProxyWindowBase::windowConnected,
			    this,
			    &HyprlandFocusGrab::onProxyConnected
			);

			if (this->grab != nullptr && newProxy->backingWindow() != nullptr) {
				this->grab->addWindow(newProxy->backingWindow());
			}
		}
	}

	this->trackedProxies = newProxy;
	if (this->grab) this->grab->completeTransaction();
}

} // namespace qs::hyprland
