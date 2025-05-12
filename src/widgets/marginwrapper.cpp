#include "marginwrapper.hpp"

#include <qobject.h>
#include <qquickitem.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "wrapper.hpp"

namespace qs::widgets {

MarginWrapperManager::MarginWrapperManager(QObject* parent): WrapperManager(parent) {
	QObject::connect(
	    this,
	    &WrapperManager::initializedChildChanged,
	    this,
	    &MarginWrapperManager::onChildChanged
	);

	this->bTopMargin.setBinding([this] {
		return this->bExtraMargin
		     + (this->bTopMarginSet.value() ? this->bTopMarginValue : this->bMargin);
	});

	this->bBottomMargin.setBinding([this] {
		return this->bExtraMargin
		     + (this->bBottomMarginSet.value() ? this->bBottomMarginValue : this->bMargin);
	});

	this->bLeftMargin.setBinding([this] {
		return this->bExtraMargin
		     + (this->bLeftMarginSet.value() ? this->bLeftMarginValue : this->bMargin);
	});

	this->bRightMargin.setBinding([this] {
		return this->bExtraMargin
		     + (this->bRightMarginSet.value() ? this->bRightMarginValue : this->bMargin);
	});

	// Coalesces updates via binding infrastructure
	this->bUpdateWatcher.setBinding([this] {
		this->bTopMargin.value();
		this->bBottomMargin.value();
		this->bLeftMargin.value();
		this->bRightMargin.value();
		return 0;
	});
}

void MarginWrapperManager::componentComplete() {
	this->WrapperManager::componentComplete();

	if (this->mWrapper) {
		QObject::connect(
		    this->mWrapper,
		    &QQuickItem::widthChanged,
		    this,
		    &MarginWrapperManager::updateChildX
		);

		QObject::connect(
		    this->mWrapper,
		    &QQuickItem::heightChanged,
		    this,
		    &MarginWrapperManager::updateChildY
		);
	}

	if (!this->mChild) this->updateGeometry();
}

bool MarginWrapperManager::resizeChild() const { return this->mResizeChild; }

void MarginWrapperManager::setResizeChild(bool resizeChild) {
	if (resizeChild == this->mResizeChild) return;
	this->mResizeChild = resizeChild;
	this->updateGeometry();
	emit this->resizeChildChanged();
}

void MarginWrapperManager::onChildChanged() {
	// QObject::disconnect in MarginWrapper handles disconnecting old item

	if (this->mChild) {
		QObject::connect(
		    this->mChild,
		    &QQuickItem::implicitWidthChanged,
		    this,
		    &MarginWrapperManager::onChildImplicitWidthChanged
		);

		QObject::connect(
		    this->mChild,
		    &QQuickItem::implicitHeightChanged,
		    this,
		    &MarginWrapperManager::onChildImplicitHeightChanged
		);
	}

	this->updateGeometry();
}

qreal MarginWrapperManager::targetChildWidth() const {
	auto max = this->mWrapper->width() - (this->bLeftMargin + this->bRightMargin);

	if (this->mResizeChild) return max;
	else return this->mChild->implicitWidth();
}

qreal MarginWrapperManager::targetChildHeight() const {
	auto max = this->mWrapper->height() - (this->bTopMargin + this->bBottomMargin);

	if (this->mResizeChild) return max;
	else return this->mChild->implicitHeight();
}

qreal MarginWrapperManager::targetChildX() const {
	if (this->mResizeChild) return this->bLeftMargin;
	else {
		auto total = this->bLeftMargin + this->bRightMargin;
		auto mul = total == 0 ? 0.5 : this->bLeftMargin / total;
		auto margin = this->mWrapper->width() - this->mChild->implicitWidth();
		return margin * mul;
	}
}

qreal MarginWrapperManager::targetChildY() const {
	if (this->mResizeChild) return this->bTopMargin;
	else {
		auto total = this->bTopMargin + this->bBottomMargin;
		auto mul = total == 0 ? 0.5 : this->bTopMargin / total;
		auto margin = this->mWrapper->height() - this->mChild->implicitHeight();
		return margin * mul;
	}
}

void MarginWrapperManager::updateChildX() {
	if (!this->mChild || !this->mWrapper) return;
	this->mChild->setX(this->targetChildX());
	this->mChild->setWidth(this->targetChildWidth());
}

void MarginWrapperManager::updateChildY() {
	if (!this->mChild || !this->mWrapper) return;
	this->mChild->setY(this->targetChildY());
	this->mChild->setHeight(this->targetChildHeight());
}

void MarginWrapperManager::onChildImplicitWidthChanged() {
	if (!this->mChild || !this->mWrapper) return;
	this->mWrapper->setImplicitWidth(
	    this->mChild->implicitWidth() + this->bLeftMargin + this->bRightMargin
	);

	// If the implicit width change does not result in an actual width change,
	// this will not be called anywhere else.
	this->updateChildX();
}

void MarginWrapperManager::onChildImplicitHeightChanged() {
	if (!this->mChild || !this->mWrapper) return;
	this->mWrapper->setImplicitHeight(
	    this->mChild->implicitHeight() + this->bTopMargin + this->bBottomMargin
	);

	// If the implicit height change does not result in an actual height change,
	// this will not be called anywhere else.
	this->updateChildY();
}

void MarginWrapperManager::updateGeometry() {
	if (!this->mWrapper) return;

	if (this->mChild) {
		this->mWrapper->setImplicitWidth(
		    this->mChild->implicitWidth() + this->bLeftMargin + this->bRightMargin
		);
		this->mWrapper->setImplicitHeight(
		    this->mChild->implicitHeight() + this->bTopMargin + this->bBottomMargin
		);
		this->mChild->setX(this->targetChildX());
		this->mChild->setY(this->targetChildY());
		this->mChild->setWidth(this->targetChildWidth());
		this->mChild->setHeight(this->targetChildHeight());
	} else {
		this->mWrapper->setImplicitWidth(this->bLeftMargin + this->bRightMargin);
		this->mWrapper->setImplicitHeight(this->bTopMargin + this->bBottomMargin);
	}
}

} // namespace qs::widgets
