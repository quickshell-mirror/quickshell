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
	QObject::connect(&this->mAnchor, &PopupAnchor::windowChanged, this, &ProxyPopupWindow::onParentWindowChanged);
	QObject::connect(&this->mAnchor, &PopupAnchor::windowRectChanged, this, &ProxyPopupWindow::reposition);
	QObject::connect(&this->mAnchor, &PopupAnchor::edgesChanged, this, &ProxyPopupWindow::reposition);
	QObject::connect(&this->mAnchor, &PopupAnchor::gravityChanged, this, &ProxyPopupWindow::reposition);
	QObject::connect(&this->mAnchor, &PopupAnchor::adjustmentChanged, this, &ProxyPopupWindow::reposition);
	// clang-format on

	this->bTargetVisible.setBinding([this] {
		auto* window = this->mAnchor.bindableProxyWindow().value();

		if (window == this) {
			qmlWarning(this) << "Anchor assigned to current window";
			return false;
		}

		if (!window) return false;

		if (!this->bWantsVisible) return false;
		return window->bindableBackerVisibility().value();
	});
}

void ProxyPopupWindow::targetVisibleChanged() {
	this->ProxyWindowBase::setVisible(this->bTargetVisible);
}

void ProxyPopupWindow::completeWindow() {
	this->ProxyWindowBase::completeWindow();

	// clang-format off
	QObject::connect(this, &ProxyWindowBase::closed, this, &ProxyPopupWindow::onClosed);
	QObject::connect(this->window, &QWindow::widthChanged, this, &ProxyPopupWindow::reposition);
	QObject::connect(this->window, &QWindow::heightChanged, this, &ProxyPopupWindow::reposition);
	// clang-format on

	auto* bw = this->mAnchor.backingWindow();

	if (bw && PopupPositioner::instance()->shouldRepositionOnMove()) {
		QObject::connect(bw, &QWindow::xChanged, this, &ProxyPopupWindow::reposition);
		QObject::connect(bw, &QWindow::yChanged, this, &ProxyPopupWindow::reposition);
		QObject::connect(bw, &QWindow::widthChanged, this, &ProxyPopupWindow::reposition);
		QObject::connect(bw, &QWindow::heightChanged, this, &ProxyPopupWindow::reposition);
	}

	this->window->setTransientParent(bw);
	this->window->setFlag(Qt::ToolTip);

	this->mAnchor.markDirty();
	PopupPositioner::instance()->reposition(&this->mAnchor, this->window);
}

void ProxyPopupWindow::postCompleteWindow() {
	this->ProxyWindowBase::setVisible(this->bTargetVisible);
}

void ProxyPopupWindow::onClosed() { this->bWantsVisible = false; }

void ProxyPopupWindow::onParentWindowChanged() {
	// recreate for new parent
	if (this->bTargetVisible && this->isVisibleDirect()) {
		this->ProxyWindowBase::setVisibleDirect(false);
		this->ProxyWindowBase::setVisibleDirect(true);
	}

	emit this->parentWindowChanged();
}

void ProxyPopupWindow::setParentWindow(QObject* parent) {
	qmlWarning(this) << "PopupWindow.parentWindow is deprecated. Use PopupWindow.anchor.window.";
	this->mAnchor.setWindow(parent);
}

QObject* ProxyPopupWindow::parentWindow() const { return this->mAnchor.window(); }

void ProxyPopupWindow::setScreen(QuickshellScreenInfo* /*unused*/) {
	qmlWarning(
	    this
	) << "Cannot set screen of popup window, as that is controlled by the parent window";
}

void ProxyPopupWindow::setVisible(bool visible) { this->bWantsVisible = visible; }

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

bool ProxyPopupWindow::deleteOnInvisible() const { return true; }
