#include "marginwrapper.hpp"
#include <cmath>

#include <qobject.h>
#include <qquickitem.h>
#include <qtmetamacros.h>

#include "wrapper.hpp"

namespace qs::widgets {

MarginWrapperManager::MarginWrapperManager(QObject* parent): WrapperManager(parent) {
	this->bTopMargin.setBinding([this] {
		return this->bExtraMargin
		     + (this->bOverrides.value().testFlag(TopMargin) ? this->bTopMarginOverride : this->bMargin
		     );
	});

	this->bBottomMargin.setBinding([this] {
		return this->bExtraMargin
		     + (this->bOverrides.value().testFlag(BottomMargin) ? this->bBottomMarginOverride
		                                                        : this->bMargin);
	});

	this->bLeftMargin.setBinding([this] {
		return this->bExtraMargin
		     + (this->bOverrides.value().testFlag(LeftMargin) ? this->bLeftMarginOverride
		                                                      : this->bMargin);
	});

	this->bRightMargin.setBinding([this] {
		return this->bExtraMargin
		     + (this->bOverrides.value().testFlag(RightMargin) ? this->bRightMarginOverride
		                                                       : this->bMargin);
	});

	this->bChildX.setBinding([this] {
		if (this->bResizeChild) return this->bLeftMargin.value();

		auto total = this->bLeftMargin + this->bRightMargin;
		auto mul = total == 0 ? 0.5 : this->bLeftMargin / total;
		auto margin = this->bWrapperWidth - this->bChildImplicitWidth;
		return std::round(margin * mul);
	});

	this->bChildY.setBinding([this] {
		if (this->bResizeChild) return this->bTopMargin.value();

		auto total = this->bTopMargin + this->bBottomMargin;
		auto mul = total == 0 ? 0.5 : this->bTopMargin / total;
		auto margin = this->bWrapperHeight - this->bChildImplicitHeight;
		return std::round(margin * mul);
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
		if (this->bOverrides.value().testFlag(ImplicitWidth)) {
			return this->bImplicitWidthOverride.value();
		} else {
			return this->bChildImplicitWidth.value() + this->bLeftMargin + this->bRightMargin;
		}
	});

	this->bWrapperImplicitHeight.setBinding([this] {
		if (this->bOverrides.value().testFlag(ImplicitHeight)) {
			return this->bImplicitHeightOverride.value();
		} else {
			return this->bChildImplicitHeight.value() + this->bTopMargin + this->bBottomMargin;
		}
	});
}

void MarginWrapperManager::componentComplete() {
	this->WrapperManager::componentComplete();

	if (this->mWrapper) {
		this->bWrapperWidth.setBinding([this] { return this->mWrapper->bindableWidth().value(); });
		this->bWrapperHeight.setBinding([this] { return this->mWrapper->bindableHeight().value(); });
		this->setWrapperImplicitWidth();
		this->setWrapperImplicitHeight();
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
	emit this->implicitWidthChanged();
}

void MarginWrapperManager::setWrapperImplicitHeight() {
	if (this->mWrapper) this->mWrapper->setImplicitHeight(this->bWrapperImplicitHeight);
	emit this->implicitHeightChanged();
}

} // namespace qs::widgets
