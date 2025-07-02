#include "popupanchor.hpp"
#include <algorithm>

#include <qcontainerfwd.h>
#include <qlogging.h>
#include <qobject.h>
#include <qquickitem.h>
#include <qsize.h>
#include <qtmetamacros.h>
#include <qvectornd.h>
#include <qwindow.h>

#include "../window/proxywindow.hpp"
#include "../window/windowinterface.hpp"
#include "types.hpp"

bool PopupAnchorState::operator==(const PopupAnchorState& other) const {
	return this->rect == other.rect && this->edges == other.edges && this->gravity == other.gravity
	    && this->adjustment == other.adjustment && this->anchorpoint == other.anchorpoint
	    && this->size == other.size;
}

bool PopupAnchor::isDirty() const {
	return !this->lastState.has_value() || this->state != this->lastState.value();
}

void PopupAnchor::markClean() { this->lastState = this->state; }
void PopupAnchor::markDirty() { this->lastState.reset(); }

QWindow* PopupAnchor::backingWindow() const {
	return this->mProxyWindow ? this->mProxyWindow->backingWindow() : nullptr;
}

void PopupAnchor::setWindowInternal(QObject* window) {
	if (window == this->mWindow) return;

	if (this->mWindow) {
		QObject::disconnect(this->mWindow, nullptr, this, nullptr);
		QObject::disconnect(this->mProxyWindow, nullptr, this, nullptr);
	}

	if (window) {
		if (auto* proxy = qobject_cast<ProxyWindowBase*>(window)) {
			this->mProxyWindow = proxy;
		} else if (auto* interface = qobject_cast<WindowInterface*>(window)) {
			this->mProxyWindow = interface->proxyWindow();
		} else {
			qWarning() << "Tried to set popup anchor window to" << window
			           << "which is not a quickshell window.";
			goto setnull;
		}

		this->mWindow = window;

		QObject::connect(this->mWindow, &QObject::destroyed, this, &PopupAnchor::onWindowDestroyed);

		QObject::connect(
		    this->mProxyWindow,
		    &ProxyWindowBase::backerVisibilityChanged,
		    this,
		    &PopupAnchor::backingWindowVisibilityChanged
		);

		emit this->windowChanged();
		emit this->backingWindowVisibilityChanged();

		return;
	}

setnull:
	if (this->mWindow) {
		this->mWindow = nullptr;
		this->mProxyWindow = nullptr;

		emit this->windowChanged();
		emit this->backingWindowVisibilityChanged();
	}
}

void PopupAnchor::setWindow(QObject* window) {
	this->setItem(nullptr);
	this->setWindowInternal(window);
}

void PopupAnchor::setItem(QQuickItem* item) {
	if (item == this->mItem) return;

	if (this->mItem) {
		QObject::disconnect(this->mItem, nullptr, this, nullptr);
	}

	this->mItem = item;
	this->onItemWindowChanged();

	if (item) {
		QObject::connect(item, &QObject::destroyed, this, &PopupAnchor::onItemDestroyed);
		QObject::connect(item, &QQuickItem::windowChanged, this, &PopupAnchor::onItemWindowChanged);
	}
}

void PopupAnchor::onWindowDestroyed() {
	this->mWindow = nullptr;
	this->mProxyWindow = nullptr;
	emit this->windowChanged();
	emit this->backingWindowVisibilityChanged();
}

void PopupAnchor::onItemDestroyed() {
	this->mItem = nullptr;
	emit this->itemChanged();
	this->setWindowInternal(nullptr);
}

void PopupAnchor::onItemWindowChanged() {
	if (auto* window = qobject_cast<ProxiedWindow*>(this->mItem->window())) {
		this->setWindowInternal(window->proxy());
	} else {
		this->setWindowInternal(nullptr);
	}
}

void PopupAnchor::setRect(Box rect) {
	if (rect.w <= 0) rect.w = 1;
	if (rect.h <= 0) rect.h = 1;
	if (rect == this->mUserRect) return;

	this->mUserRect = rect;
	emit this->rectChanged();

	this->setWindowRect(rect.qrect().marginsRemoved(this->mMargins.qmargins()));
}

void PopupAnchor::setMargins(Margins margins) {
	if (margins == this->mMargins) return;

	this->mMargins = margins;
	emit this->marginsChanged();

	this->setWindowRect(this->mUserRect.qrect().marginsRemoved(margins.qmargins()));
}

void PopupAnchor::setWindowRect(QRect rect) {
	if (rect.width() <= 0) rect.setWidth(1);
	if (rect.height() <= 0) rect.setHeight(1);
	if (rect == this->state.rect) return;

	this->state.rect = rect;
	emit this->windowRectChanged();
}

void PopupAnchor::resetRect() { this->mUserRect = Box(); }

void PopupAnchor::setEdges(Edges::Flags edges) {
	if (edges == this->state.edges) return;

	if (Edges::isOpposing(edges)) {
		qWarning() << "Cannot set opposing edges for anchor edges. Tried to set" << edges;
		return;
	}

	this->state.edges = edges;
	emit this->edgesChanged();
}

void PopupAnchor::setGravity(Edges::Flags gravity) {
	if (gravity == this->state.gravity) return;

	if (Edges::isOpposing(gravity)) {
		qWarning() << "Cannot set opposing edges for anchor gravity. Tried to set" << gravity;
		return;
	}

	this->state.gravity = gravity;
	emit this->gravityChanged();
}

void PopupAnchor::setAdjustment(PopupAdjustment::Flags adjustment) {
	if (adjustment == this->state.adjustment) return;
	this->state.adjustment = adjustment;
	emit this->adjustmentChanged();
}

void PopupAnchor::updatePlacement(const QPoint& anchorpoint, const QSize& size) {
	this->state.anchorpoint = anchorpoint;
	this->state.size = size;
}

void PopupAnchor::updateAnchor() {
	if (this->mItem && this->mProxyWindow) {
		auto baseRect =
		    this->mUserRect.isEmpty() ? this->mItem->boundingRect() : this->mUserRect.qrect();

		auto rect = this->mProxyWindow->contentItem()->mapFromItem(
		    this->mItem,
		    baseRect.marginsRemoved(this->mMargins.qmargins())
		);

		if (rect.width() < 1) rect.setWidth(1);
		if (rect.height() < 1) rect.setHeight(1);

		this->setWindowRect(rect.toRect());
	}

	emit this->anchoring();
}

static PopupPositioner* POSITIONER = nullptr; // NOLINT

void PopupPositioner::reposition(PopupAnchor* anchor, QWindow* window, bool onlyIfDirty) {
	auto* parentWindow = window->transientParent();
	if (!parentWindow) {
		qFatal() << "Cannot reposition popup that does not have a transient parent.";
	}

	auto parentGeometry = parentWindow->geometry();
	auto windowGeometry = window->geometry();

	anchor->updateAnchor();
	anchor->updatePlacement(parentGeometry.topLeft(), windowGeometry.size());

	if (onlyIfDirty && !anchor->isDirty()) return;
	anchor->markClean();

	auto adjustment = anchor->adjustment();
	auto screenGeometry = parentWindow->screen()->geometry();
	auto anchorRectGeometry = anchor->windowRect().translated(parentGeometry.topLeft());

	auto anchorEdges = anchor->edges();
	auto anchorGravity = anchor->gravity();

	auto width = windowGeometry.width();
	auto height = windowGeometry.height();

	auto anchorX = anchorEdges.testFlag(Edges::Left)  ? anchorRectGeometry.left()
	             : anchorEdges.testFlag(Edges::Right) ? anchorRectGeometry.right()
	                                                  : anchorRectGeometry.center().x();

	auto anchorY = anchorEdges.testFlag(Edges::Top)    ? anchorRectGeometry.top()
	             : anchorEdges.testFlag(Edges::Bottom) ? anchorRectGeometry.bottom()
	                                                   : anchorRectGeometry.center().y();

	auto calcEffectiveX = [&](Edges::Flags anchorGravity, int anchorX) {
		auto ex = anchorGravity.testFlag(Edges::Left)  ? anchorX - windowGeometry.width()
		        : anchorGravity.testFlag(Edges::Right) ? anchorX - 1
		                                               : anchorX - windowGeometry.width() / 2;

		return ex + 1;
	};

	auto calcEffectiveY = [&](Edges::Flags anchorGravity, int anchorY) {
		auto ey = anchorGravity.testFlag(Edges::Top)    ? anchorY - windowGeometry.height()
		        : anchorGravity.testFlag(Edges::Bottom) ? anchorY - 1
		                                                : anchorY - windowGeometry.height() / 2;

		return ey + 1;
	};

	auto calcRemainingWidth = [&](int effectiveX) {
		auto width = windowGeometry.width();
		if (effectiveX < screenGeometry.left()) {
			auto diff = screenGeometry.left() - effectiveX;
			effectiveX = screenGeometry.left();
			width -= diff;
		}

		auto effectiveX2 = effectiveX + width;
		if (effectiveX2 > screenGeometry.right()) {
			width -= effectiveX2 - screenGeometry.right() - 1;
		}

		return QPair<int, int>(effectiveX, width);
	};

	auto calcRemainingHeight = [&](int effectiveY) {
		auto height = windowGeometry.height();
		if (effectiveY < screenGeometry.left()) {
			auto diff = screenGeometry.top() - effectiveY;
			effectiveY = screenGeometry.top();
			height -= diff;
		}

		auto effectiveY2 = effectiveY + height;
		if (effectiveY2 > screenGeometry.bottom()) {
			height -= effectiveY2 - screenGeometry.bottom() - 1;
		}

		return QPair<int, int>(effectiveY, height);
	};

	auto effectiveX = calcEffectiveX(anchorGravity, anchorX);
	auto effectiveY = calcEffectiveY(anchorGravity, anchorY);

	if (adjustment.testFlag(PopupAdjustment::FlipX)) {
		const bool flip = (anchorGravity.testFlag(Edges::Left) && effectiveX < screenGeometry.left())
		               || (anchorGravity.testFlag(Edges::Right)
		                   && effectiveX + windowGeometry.width() > screenGeometry.right());

		if (flip) {
			auto newAnchorGravity = anchorGravity ^ (Edges::Left | Edges::Right);

			auto newAnchorX = anchorEdges.testFlags(Edges::Left)  ? anchorRectGeometry.right()
			                : anchorEdges.testFlags(Edges::Right) ? anchorRectGeometry.left()
			                                                      : anchorX;

			auto newEffectiveX = calcEffectiveX(newAnchorGravity, newAnchorX);

			// TODO IN HL: pick constraint monitor based on anchor rect position in window

			// if the available width when flipped is more than the available width without flipping then flip
			if (calcRemainingWidth(newEffectiveX).second > calcRemainingWidth(effectiveX).second) {
				anchorGravity = newAnchorGravity;
				anchorX = newAnchorX;
				effectiveX = newEffectiveX;
			}
		}
	}

	if (adjustment.testFlag(PopupAdjustment::FlipY)) {
		const bool flip = (anchorGravity.testFlag(Edges::Top) && effectiveY < screenGeometry.top())
		               || (anchorGravity.testFlag(Edges::Bottom)
		                   && effectiveY + windowGeometry.height() > screenGeometry.bottom());

		if (flip) {
			auto newAnchorGravity = anchorGravity ^ (Edges::Top | Edges::Bottom);

			auto newAnchorY = anchorEdges.testFlags(Edges::Top)    ? anchorRectGeometry.bottom()
			                : anchorEdges.testFlags(Edges::Bottom) ? anchorRectGeometry.top()
			                                                       : anchorY;

			auto newEffectiveY = calcEffectiveY(newAnchorGravity, newAnchorY);

			// if the available width when flipped is more than the available width without flipping then flip
			if (calcRemainingHeight(newEffectiveY).second > calcRemainingHeight(effectiveY).second) {
				anchorGravity = newAnchorGravity;
				anchorY = newAnchorY;
				effectiveY = newEffectiveY;
			}
		}
	}

	// Slide order is important for the case where the window is too large to fit on screen.
	if (adjustment.testFlag(PopupAdjustment::SlideX)) {
		if (effectiveX + windowGeometry.width() > screenGeometry.right()) {
			effectiveX = screenGeometry.right() - windowGeometry.width() + 1;
		}

		effectiveX = std::max(effectiveX, screenGeometry.left());
	}

	if (adjustment.testFlag(PopupAdjustment::SlideY)) {
		if (effectiveY + windowGeometry.height() > screenGeometry.bottom()) {
			effectiveY = screenGeometry.bottom() - windowGeometry.height() + 1;
		}

		effectiveY = std::max(effectiveY, screenGeometry.top());
	}

	if (adjustment.testFlag(PopupAdjustment::ResizeX)) {
		auto [newX, newWidth] = calcRemainingWidth(effectiveX);
		effectiveX = newX;
		width = newWidth;
	}

	if (adjustment.testFlag(PopupAdjustment::ResizeY)) {
		auto [newY, newHeight] = calcRemainingHeight(effectiveY);
		effectiveY = newY;
		height = newHeight;
	}

	window->setGeometry({effectiveX, effectiveY, width, height});
}

bool PopupPositioner::shouldRepositionOnMove() const { return true; }

PopupPositioner* PopupPositioner::instance() {
	if (POSITIONER == nullptr) {
		POSITIONER = new PopupPositioner();
	}

	return POSITIONER;
}

void PopupPositioner::setInstance(PopupPositioner* instance) {
	delete POSITIONER;
	POSITIONER = instance;
}
