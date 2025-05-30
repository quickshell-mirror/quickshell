#include "popupwindow.hpp"

#include <qnamespace.h>
#include <qobject.h>
#include <qqmlinfo.h>
#include <qtypes.h>
#include <qwindow.h>

#include "../core/popupanchor.hpp"
#include "../core/qmlscreen.hpp"
#include "windowinterface.hpp"

ProxyPopupWindow::ProxyPopupWindow(QObject* parent): ProxyWindowBase(parent) {
	this->mVisible = false;
	// clang-format off
	QObject::connect(&this->mAnchor, &PopupAnchor::windowChanged, this, &ProxyPopupWindow::parentWindowChanged);
	QObject::connect(&this->mAnchor, &PopupAnchor::windowRectChanged, this, &ProxyPopupWindow::reposition);
	QObject::connect(&this->mAnchor, &PopupAnchor::edgesChanged, this, &ProxyPopupWindow::reposition);
	QObject::connect(&this->mAnchor, &PopupAnchor::gravityChanged, this, &ProxyPopupWindow::reposition);
	QObject::connect(&this->mAnchor, &PopupAnchor::adjustmentChanged, this, &ProxyPopupWindow::reposition);
	QObject::connect(&this->mAnchor, &PopupAnchor::backingWindowVisibilityChanged, this, &ProxyPopupWindow::onParentUpdated);
	// clang-format on
}

void ProxyPopupWindow::completeWindow() {
	this->ProxyWindowBase::completeWindow();

	// clang-format off
	QObject::connect(this->window, &QWindow::visibleChanged, this, &ProxyPopupWindow::onVisibleChanged);
	QObject::connect(this->window, &QWindow::widthChanged, this, &ProxyPopupWindow::reposition);
	QObject::connect(this->window, &QWindow::heightChanged, this, &ProxyPopupWindow::reposition);
	// clang-format on

	this->window->setFlag(Qt::ToolTip);
}

void ProxyPopupWindow::postCompleteWindow() { this->updateTransientParent(); }

void ProxyPopupWindow::setParentWindow(QObject* parent) {
	qmlWarning(this) << "PopupWindow.parentWindow is deprecated. Use PopupWindow.anchor.window.";
	this->mAnchor.setWindow(parent);
}

QObject* ProxyPopupWindow::parentWindow() const { return this->mAnchor.window(); }

void ProxyPopupWindow::updateTransientParent() {
	auto* bw = this->mAnchor.backingWindow();

	if (this->window != nullptr && bw != this->window->transientParent()) {
		if (this->window->transientParent()) {
			QObject::disconnect(this->window->transientParent(), nullptr, this, nullptr);
		}

		if (bw && PopupPositioner::instance()->shouldRepositionOnMove()) {
			QObject::connect(bw, &QWindow::xChanged, this, &ProxyPopupWindow::reposition);
			QObject::connect(bw, &QWindow::yChanged, this, &ProxyPopupWindow::reposition);
			QObject::connect(bw, &QWindow::widthChanged, this, &ProxyPopupWindow::reposition);
			QObject::connect(bw, &QWindow::heightChanged, this, &ProxyPopupWindow::reposition);
		}

		this->window->setTransientParent(bw);
	}

	this->updateVisible();
}

void ProxyPopupWindow::onParentUpdated() { this->updateTransientParent(); }

void ProxyPopupWindow::setScreen(QuickshellScreenInfo* /*unused*/) {
	qmlWarning(this
	) << "Cannot set screen of popup window, as that is controlled by the parent window";
}

void ProxyPopupWindow::setVisible(bool visible) {
	if (visible == this->wantsVisible) return;
	this->wantsVisible = visible;
	this->updateVisible();
}

void ProxyPopupWindow::updateVisible() {
	auto target = this->wantsVisible && this->mAnchor.window() != nullptr
	           && this->mAnchor.proxyWindow()->isVisibleDirect();

	if (target && this->window != nullptr && !this->window->isVisible()) {
		PopupPositioner::instance()->reposition(&this->mAnchor, this->window);
	}

	this->ProxyWindowBase::setVisible(target);
}

void ProxyPopupWindow::onVisibleChanged() {
	// If the window was made invisible without its parent becoming invisible
	// the compositor probably destroyed it. Without this the window won't ever
	// be able to become visible again.
	if (this->window->transientParent() && this->window->transientParent()->isVisible()) {
		this->wantsVisible = this->window->isVisible();
	}
}

void ProxyPopupWindow::setRelativeX(qint32 x) {
	qmlWarning(this) << "PopupWindow.relativeX is deprecated. Use PopupWindow.anchor.rect.x.";
	auto rect = this->mAnchor.rect();
	if (x == rect.x) return;
	rect.x = x;
	this->mAnchor.setRect(rect);
}

qint32 ProxyPopupWindow::relativeX() const {
	qmlWarning(this) << "PopupWindow.relativeX is deprecated. Use PopupWindow.anchor.rect.x.";
	return this->mAnchor.rect().x;
}

void ProxyPopupWindow::setRelativeY(qint32 y) {
	qmlWarning(this) << "PopupWindow.relativeY is deprecated. Use PopupWindow.anchor.rect.y.";
	auto rect = this->mAnchor.rect();
	if (y == rect.y) return;
	rect.y = y;
	this->mAnchor.setRect(rect);
}

qint32 ProxyPopupWindow::relativeY() const {
	qmlWarning(this) << "PopupWindow.relativeY is deprecated. Use PopupWindow.anchor.rect.y.";
	return this->mAnchor.rect().y;
}

PopupAnchor* ProxyPopupWindow::anchor() { return &this->mAnchor; }

void ProxyPopupWindow::reposition() {
	// not gated on pendingReposition as a polish might not be triggered in edge cases
	if (this->window) {
		this->pendingReposition = true;
		this->schedulePolish();
	}
}

void ProxyPopupWindow::onPolished() {
	this->ProxyWindowBase::onPolished();
	if (this->pendingReposition) {
		this->pendingReposition = false;

		if (this->window) {
			PopupPositioner::instance()->reposition(&this->mAnchor, this->window);
		}
	}
}
