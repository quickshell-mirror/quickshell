#include "popupanchor.hpp"

#include <qlogging.h>
#include <qobject.h>
#include <qsize.h>
#include <qtmetamacros.h>
#include <qwindow.h>

#include "proxywindow.hpp"
#include "types.hpp"
#include "windowinterface.hpp"

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

QObject* PopupAnchor::window() const { return this->mWindow; }
ProxyWindowBase* PopupAnchor::proxyWindow() const { return this->mProxyWindow; }

QWindow* PopupAnchor::backingWindow() const {
	return this->mProxyWindow ? this->mProxyWindow->backingWindow() : nullptr;
}

void PopupAnchor::setWindow(QObject* window) {
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

void PopupAnchor::onWindowDestroyed() {
	this->mWindow = nullptr;
	this->mProxyWindow = nullptr;
	emit this->windowChanged();
	emit this->backingWindowVisibilityChanged();
}

Box PopupAnchor::rect() const { return this->state.rect; }

void PopupAnchor::setRect(Box rect) {
	if (rect == this->state.rect) return;
	if (rect.w <= 0) rect.w = 1;
	if (rect.h <= 0) rect.h = 1;

	this->state.rect = rect;
	emit this->rectChanged();
}

Edges::Flags PopupAnchor::edges() const { return this->state.edges; }

void PopupAnchor::setEdges(Edges::Flags edges) {
	if (edges == this->state.edges) return;

	if (Edges::isOpposing(edges)) {
		qWarning() << "Cannot set opposing edges for anchor edges. Tried to set" << edges;
		return;
	}

	this->state.edges = edges;
	emit this->edgesChanged();
}

Edges::Flags PopupAnchor::gravity() const { return this->state.gravity; }

void PopupAnchor::setGravity(Edges::Flags gravity) {
	if (gravity == this->state.gravity) return;

	if (Edges::isOpposing(gravity)) {
		qWarning() << "Cannot set opposing edges for anchor gravity. Tried to set" << gravity;
		return;
	}

	this->state.gravity = gravity;
	emit this->gravityChanged();
}

PopupAdjustment::Flags PopupAnchor::adjustment() const { return this->state.adjustment; }

void PopupAnchor::setAdjustment(PopupAdjustment::Flags adjustment) {
	if (adjustment == this->state.adjustment) return;
	this->state.adjustment = adjustment;
	emit this->adjustmentChanged();
}

void PopupAnchor::updatePlacement(const QPoint& anchorpoint, const QSize& size) {
	this->state.anchorpoint = anchorpoint;
	this->state.size = size;
}

static PopupPositioner* POSITIONER = nullptr; // NOLINT

void PopupPositioner::reposition(PopupAnchor* anchor, QWindow* window, bool onlyIfDirty) {
	auto* parentWindow = window->transientParent();
	if (!parentWindow) {
		qFatal() << "Cannot reposition popup that does not have a transient parent.";
	}

	auto parentGeometry = parentWindow->geometry();
	auto windowGeometry = window->geometry();

	anchor->updatePlacement(parentGeometry.topLeft(), windowGeometry.size());
	if (onlyIfDirty && !anchor->isDirty()) return;
	anchor->markClean();

	auto adjustment = anchor->adjustment();
	auto screenGeometry = parentWindow->screen()->geometry();
	auto anchorRectGeometry = anchor->rect().qrect().translated(parentGeometry.topLeft());

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

	auto calcEffectiveX = [&]() {
		return anchorGravity.testFlag(Edges::Left)  ? anchorX - windowGeometry.width() + 1
		     : anchorGravity.testFlag(Edges::Right) ? anchorX
		                                            : anchorX - windowGeometry.width() / 2;
	};

	auto calcEffectiveY = [&]() {
		return anchorGravity.testFlag(Edges::Top)    ? anchorY - windowGeometry.height() + 1
		     : anchorGravity.testFlag(Edges::Bottom) ? anchorY
		                                             : anchorY - windowGeometry.height() / 2;
	};

	auto effectiveX = calcEffectiveX();
	auto effectiveY = calcEffectiveY();

	if (adjustment.testFlag(PopupAdjustment::FlipX)) {
		if (anchorGravity.testFlag(Edges::Left)) {
			if (effectiveX < screenGeometry.left()) {
				anchorGravity = anchorGravity ^ Edges::Left | Edges::Right;
				anchorX = anchorRectGeometry.right();
				effectiveX = calcEffectiveX();
			}
		} else if (anchorGravity.testFlag(Edges::Right)) {
			if (effectiveX + windowGeometry.width() > screenGeometry.right()) {
				anchorGravity = anchorGravity ^ Edges::Right | Edges::Left;
				anchorX = anchorRectGeometry.left();
				effectiveX = calcEffectiveX();
			}
		}
	}

	if (adjustment.testFlag(PopupAdjustment::FlipY)) {
		if (anchorGravity.testFlag(Edges::Top)) {
			if (effectiveY < screenGeometry.top()) {
				anchorGravity = anchorGravity ^ Edges::Top | Edges::Bottom;
				effectiveY = calcEffectiveY();
			}
		} else if (anchorGravity.testFlag(Edges::Bottom)) {
			if (effectiveY + windowGeometry.height() > screenGeometry.bottom()) {
				anchorGravity = anchorGravity ^ Edges::Bottom | Edges::Top;
				effectiveY = calcEffectiveY();
			}
		}
	}

	// Slide order is important for the case where the window is too large to fit on screen.
	if (adjustment.testFlag(PopupAdjustment::SlideX)) {
		if (effectiveX + windowGeometry.width() > screenGeometry.right()) {
			effectiveX = screenGeometry.right() - windowGeometry.width() + 1;
		}

		if (effectiveX < screenGeometry.left()) {
			effectiveX = screenGeometry.left();
		}
	}

	if (adjustment.testFlag(PopupAdjustment::SlideY)) {
		if (effectiveY + windowGeometry.height() > screenGeometry.bottom()) {
			effectiveY = screenGeometry.bottom() - windowGeometry.height() + 1;
		}

		if (effectiveY < screenGeometry.top()) {
			effectiveY = screenGeometry.top();
		}
	}

	if (adjustment.testFlag(PopupAdjustment::ResizeX)) {
		if (effectiveX < screenGeometry.left()) {
			auto diff = screenGeometry.left() - effectiveX;
			effectiveX = screenGeometry.left();
			width -= diff;
		}

		auto effectiveX2 = effectiveX + windowGeometry.width();
		if (effectiveX2 > screenGeometry.right()) {
			width -= effectiveX2 - screenGeometry.right() - 1;
		}
	}

	if (adjustment.testFlag(PopupAdjustment::ResizeY)) {
		if (effectiveY < screenGeometry.top()) {
			auto diff = screenGeometry.top() - effectiveY;
			effectiveY = screenGeometry.top();
			height -= diff;
		}

		auto effectiveY2 = effectiveY + windowGeometry.height();
		if (effectiveY2 > screenGeometry.bottom()) {
			height -= effectiveY2 - screenGeometry.bottom() - 1;
		}
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
