#include "marginwrapper.hpp"
#include <algorithm>

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

qreal MarginWrapperManager::margin() const { return this->mMargin; }

void MarginWrapperManager::setMargin(qreal margin) {
	if (margin == this->mMargin) return;
	this->mMargin = margin;
	this->updateGeometry();
	emit this->marginChanged();
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
	auto max = this->mWrapper->width() - this->mMargin * 2;

	if (this->mResizeChild) return max;
	else return std::min(this->mChild->implicitWidth(), max);
}

qreal MarginWrapperManager::targetChildHeight() const {
	auto max = this->mWrapper->height() - this->mMargin * 2;

	if (this->mResizeChild) return max;
	else return std::min(this->mChild->implicitHeight(), max);
}

qreal MarginWrapperManager::targetChildX() const {
	if (this->mResizeChild) return this->mMargin;
	else {
		return std::max(this->mMargin, this->mWrapper->width() / 2 - this->mChild->implicitWidth() / 2);
	}
}

qreal MarginWrapperManager::targetChildY() const {
	if (this->mResizeChild) return this->mMargin;
	else {
		return std::max(
		    this->mMargin,
		    this->mWrapper->height() / 2 - this->mChild->implicitHeight() / 2
		);
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
	this->mWrapper->setImplicitWidth(this->mChild->implicitWidth() + this->mMargin * 2);

	// If the implicit width change does not result in an actual width change,
	// this will not be called anywhere else.
	this->updateChildX();
}

void MarginWrapperManager::onChildImplicitHeightChanged() {
	if (!this->mChild || !this->mWrapper) return;
	this->mWrapper->setImplicitHeight(this->mChild->implicitHeight() + this->mMargin * 2);

	// If the implicit height change does not result in an actual height change,
	// this will not be called anywhere else.
	this->updateChildY();
}

void MarginWrapperManager::updateGeometry() {
	if (!this->mWrapper) return;

	if (this->mChild) {
		this->mWrapper->setImplicitWidth(this->mChild->implicitWidth() + this->mMargin * 2);
		this->mWrapper->setImplicitHeight(this->mChild->implicitHeight() + this->mMargin * 2);
		this->mChild->setX(this->targetChildX());
		this->mChild->setY(this->targetChildY());
		this->mChild->setWidth(this->targetChildWidth());
		this->mChild->setHeight(this->targetChildHeight());
	} else {
		this->mWrapper->setImplicitWidth(this->mMargin * 2);
		this->mWrapper->setImplicitHeight(this->mMargin * 2);
	}
}

} // namespace qs::widgets
