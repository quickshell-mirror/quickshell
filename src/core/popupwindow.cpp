#include "popupwindow.hpp"

#include <qlogging.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qquickwindow.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "proxywindow.hpp"
#include "qmlscreen.hpp"
#include "windowinterface.hpp"

ProxyPopupWindow::ProxyPopupWindow(QObject* parent): ProxyWindowBase(parent) {
	this->mVisible = false;
}

void ProxyPopupWindow::completeWindow() {
	this->ProxyWindowBase::completeWindow();

	this->window->setFlag(Qt::ToolTip);
	this->updateTransientParent();
}

void ProxyPopupWindow::postCompleteWindow() {}

bool ProxyPopupWindow::deleteOnInvisible() const {
	// Currently crashes in normal mode, do not have the time to debug it now.
	return true;
}

qint32 ProxyPopupWindow::x() const {
	// QTBUG-121550
	auto basepos = this->mParentProxyWindow == nullptr ? 0 : this->mParentProxyWindow->x();
	return basepos + this->mRelativeX;
}

void ProxyPopupWindow::setParentWindow(QObject* parent) {
	if (parent == this->mParentWindow) return;

	if (this->mParentWindow != nullptr) {
		QObject::disconnect(this->mParentWindow, nullptr, this, nullptr);
		QObject::disconnect(this->mParentProxyWindow, nullptr, this, nullptr);
	}

	if (parent == nullptr) {
		this->mParentWindow = nullptr;
		this->mParentProxyWindow = nullptr;
	} else {
		if (auto* proxy = qobject_cast<ProxyWindowBase*>(parent)) {
			this->mParentProxyWindow = proxy;
		} else if (auto* interface = qobject_cast<WindowInterface*>(parent)) {
			this->mParentProxyWindow = interface->proxyWindow();
		} else {
			qWarning() << "Tried to set popup parent window to something that is not a quickshell window:"
			           << parent;
			this->mParentWindow = nullptr;
			this->mParentProxyWindow = nullptr;
			this->updateTransientParent();
			return;
		}

		this->mParentWindow = parent;

		// clang-format off
		QObject::connect(this->mParentWindow, &QObject::destroyed, this, &ProxyPopupWindow::onParentDestroyed);

		QObject::connect(this->mParentProxyWindow, &ProxyWindowBase::xChanged, this, &ProxyPopupWindow::updateX);
		QObject::connect(this->mParentProxyWindow, &ProxyWindowBase::yChanged, this, &ProxyPopupWindow::updateY);
		QObject::connect(this->mParentProxyWindow, &ProxyWindowBase::backerVisibilityChanged, this, &ProxyPopupWindow::onParentUpdated);
		// clang-format on
	}

	this->updateTransientParent();
}

QObject* ProxyPopupWindow::parentWindow() const { return this->mParentWindow; }

void ProxyPopupWindow::updateTransientParent() {
	this->updateX();
	this->updateY();

	if (this->window != nullptr) {
		this->window->setTransientParent(
		    this->mParentProxyWindow == nullptr ? nullptr : this->mParentProxyWindow->backingWindow()
		);
	}

	this->updateVisible();
}

void ProxyPopupWindow::onParentUpdated() { this->updateTransientParent(); }

void ProxyPopupWindow::onParentDestroyed() {
	this->mParentWindow = nullptr;
	this->mParentProxyWindow = nullptr;
	this->updateVisible();
	emit this->parentWindowChanged();
}

void ProxyPopupWindow::setScreen(QuickshellScreenInfo* /*unused*/) {
	qWarning() << "Cannot set screen of popup window, as that is controlled by the parent window";
}

void ProxyPopupWindow::setVisible(bool visible) {
	if (visible == this->wantsVisible) return;
	this->wantsVisible = visible;
	this->updateVisible();
}

void ProxyPopupWindow::updateVisible() {
	auto target = this->wantsVisible && this->mParentWindow != nullptr
	           && this->mParentProxyWindow->isVisibleDirect();

	if (target && this->window != nullptr && !this->window->isVisible()) {
		this->updateX(); // QTBUG-121550
	}

	this->ProxyWindowBase::setVisible(target);
}

void ProxyPopupWindow::setRelativeX(qint32 x) {
	if (x == this->mRelativeX) return;
	this->mRelativeX = x;
	this->updateX();
}

qint32 ProxyPopupWindow::relativeX() const { return this->mRelativeX; }

void ProxyPopupWindow::setRelativeY(qint32 y) {
	if (y == this->mRelativeY) return;
	this->mRelativeY = y;
	this->updateY();
}

qint32 ProxyPopupWindow::relativeY() const { return this->mRelativeY; }

void ProxyPopupWindow::updateX() {
	if (this->mParentWindow == nullptr || this->window == nullptr) return;

	auto target = this->x() - 1; // QTBUG-121550

	auto reshow = this->isVisibleDirect() && (this->window->x() != target && this->x() != target);
	if (reshow) this->setVisibleDirect(false);
	if (this->window != nullptr) this->window->setX(target);
	if (reshow && this->wantsVisible) this->setVisibleDirect(true);
}

void ProxyPopupWindow::updateY() {
	if (this->mParentWindow == nullptr || this->window == nullptr) return;

	auto target = this->mParentProxyWindow->y() + this->relativeY();

	auto reshow = this->isVisibleDirect() && this->window->y() != target;
	if (reshow) {
		this->setVisibleDirect(false);
		this->updateX(); // QTBUG-121550
	}
	if (this->window != nullptr) this->window->setY(target);
	if (reshow && this->wantsVisible) this->setVisibleDirect(true);
}
