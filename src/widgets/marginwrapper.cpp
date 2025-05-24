#include "marginwrapper.hpp"

#include <qobject.h>
#include <qquickitem.h>

#include "wrapper.hpp"

namespace qs::widgets {

MarginWrapperManager::MarginWrapperManager(QObject* parent): WrapperManager(parent) {
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

	this->bChildX.setBinding([this] {
		if (this->bResizeChild) return this->bLeftMargin.value();

		auto total = this->bLeftMargin + this->bRightMargin;
		auto mul = total == 0 ? 0.5 : this->bLeftMargin / total;
		auto margin = this->bWrapperWidth - this->bChildImplicitWidth;
		return margin * mul;
	});

	this->bChildY.setBinding([this] {
		if (this->bResizeChild) return this->bTopMargin.value();

		auto total = this->bTopMargin + this->bBottomMargin;
		auto mul = total == 0 ? 0.5 : this->bTopMargin / total;
		auto margin = this->bWrapperHeight - this->bChildImplicitHeight;
		return margin * mul;
	});

	this->bChildWidth.setBinding([this] {
		if (this->bResizeChild) {
			return this->bWrapperWidth - (this->bLeftMargin + this->bRightMargin);
		} else {
			return this->bChildImplicitWidth.value();
		}
	});

	this->bChildHeight.setBinding([this] {
		if (this->bResizeChild) {
			return this->bWrapperHeight - (this->bTopMargin + this->bBottomMargin);
		} else {
			return this->bChildImplicitHeight.value();
		}
	});

	this->bWrapperImplicitWidth.setBinding([this] {
		return this->bChildImplicitWidth.value() + this->bLeftMargin + this->bRightMargin;
	});

	this->bWrapperImplicitHeight.setBinding([this] {
		return this->bChildImplicitHeight.value() + this->bTopMargin + this->bBottomMargin;
	});
}

void MarginWrapperManager::componentComplete() {
	this->WrapperManager::componentComplete();

	if (this->mWrapper) {
		this->bWrapperWidth.setBinding([this] { return this->mWrapper->bindableWidth().value(); });
		this->bWrapperHeight.setBinding([this] { return this->mWrapper->bindableHeight().value(); });
	}
}

void MarginWrapperManager::disconnectChild() {
	this->mChild->bindableX().setValue(0);
	this->mChild->bindableY().setValue(0);
	this->mChild->bindableWidth().setValue(0);
	this->mChild->bindableHeight().setValue(0);
}

void MarginWrapperManager::connectChild() {
	// QObject::disconnect in MarginWrapper handles disconnecting old item
	this->mChild->bindableX().setBinding([this] { return this->bChildX.value(); });
	this->mChild->bindableY().setBinding([this] { return this->bChildY.value(); });
	this->mChild->bindableWidth().setBinding([this] { return this->bChildWidth.value(); });
	this->mChild->bindableHeight().setBinding([this] { return this->bChildHeight.value(); });

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

	this->onChildImplicitWidthChanged();
	this->onChildImplicitHeightChanged();
}

void MarginWrapperManager::onChildImplicitWidthChanged() {
	this->bChildImplicitWidth = this->mChild->implicitWidth();
}

void MarginWrapperManager::onChildImplicitHeightChanged() {
	this->bChildImplicitHeight = this->mChild->implicitHeight();
}

void MarginWrapperManager::setWrapperImplicitWidth() {
	if (this->mWrapper) this->mWrapper->setImplicitWidth(this->bWrapperImplicitWidth);
}

void MarginWrapperManager::setWrapperImplicitHeight() {
	if (this->mWrapper) this->mWrapper->setImplicitHeight(this->bWrapperImplicitHeight);
}

} // namespace qs::widgets
