#include "region.hpp"
#include <algorithm>
#include <cmath>

#include <qobject.h>
#include <qpoint.h>
#include <qqmllist.h>
#include <qquickitem.h>
#include <qregion.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvectornd.h>

PendingRegion::PendingRegion(QObject* parent): QObject(parent) {
	QObject::connect(this, &PendingRegion::shapeChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::intersectionChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::itemChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::xChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::yChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::widthChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::heightChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::radiusChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::topLeftCornerChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::topRightCornerChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::bottomLeftCornerChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::bottomRightCornerChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::childrenChanged, this, &PendingRegion::changed);
}

void PendingRegion::setItem(QQuickItem* item) {
	if (item == this->mItem) return;

	if (this->mItem != nullptr) {
		QObject::disconnect(this->mItem, nullptr, this, nullptr);
	}

	this->mItem = item;

	if (item != nullptr) {
		QObject::connect(this->mItem, &QObject::destroyed, this, &PendingRegion::onItemDestroyed);
		QObject::connect(this->mItem, &QQuickItem::xChanged, this, &PendingRegion::itemChanged);
		QObject::connect(this->mItem, &QQuickItem::yChanged, this, &PendingRegion::itemChanged);
		QObject::connect(this->mItem, &QQuickItem::widthChanged, this, &PendingRegion::itemChanged);
		QObject::connect(this->mItem, &QQuickItem::heightChanged, this, &PendingRegion::itemChanged);
	}

	emit this->itemChanged();
}

void PendingRegion::onItemDestroyed() { this->mItem = nullptr; }

void PendingRegion::onChildDestroyed() { this->mRegions.removeAll(this->sender()); }

QQmlListProperty<PendingRegion> PendingRegion::regions() {
	return QQmlListProperty<PendingRegion>(
	    this,
	    nullptr,
	    &PendingRegion::regionsAppend,
	    &PendingRegion::regionsCount,
	    &PendingRegion::regionAt,
	    &PendingRegion::regionsClear,
	    &PendingRegion::regionsReplace,
	    &PendingRegion::regionsRemoveLast
	);
}

bool PendingRegion::empty() const {
	return this->mItem == nullptr && this->mX == 0 && this->mY == 0 && this->mWidth == 0
	    && this->mHeight == 0;
}

QRegion PendingRegion::build() const {
	auto type = QRegion::Rectangle;
	switch (this->mShape) {
	case RegionShape::Rect: type = QRegion::Rectangle; break;
	case RegionShape::Ellipse: type = QRegion::Ellipse; break;
	}

	QRegion region;

	if (this->empty()) {
		region = QRegion();
	} else if (this->mItem != nullptr) {
		auto origin = this->mItem->mapToScene(QPointF(0, 0));
		auto extent = this->mItem->mapToScene(QPointF(this->mItem->width(), this->mItem->height()));
		auto size = extent - origin;

		region = QRegion(
		    static_cast<int>(origin.x()),
		    static_cast<int>(origin.y()),
		    static_cast<int>(std::ceil(size.x())),
		    static_cast<int>(std::ceil(size.y())),
		    type
		);
	} else {
		region = QRegion(this->mX, this->mY, this->mWidth, this->mHeight, type);
	}

	if (this->mShape == RegionShape::Rect && this->mRadius > 0 && !region.isEmpty()) {
		auto rect = region.boundingRect();
		auto x = rect.x();
		auto y = rect.y();
		auto w = rect.width();
		auto h = rect.height();
		auto r = std::min(this->mRadius, std::min(w, h) / 2);

		// Concave width at distance d from the base of the corner arc.
		// At d=0 (base, rect edge): width = r. At d=r (tip): width = 0.
		// Used for both Normal cuts (subtract from inside) and Inverted extensions (add outside).
		auto cornerWidth = [](int r, int d) -> int {
			auto dd = static_cast<double>(r - d) - 0.5;
			return r - static_cast<int>(std::round(std::sqrt(static_cast<double>(r) * r - dd * dd)));
		};

		for (int i = 0; i < r; i++) {
			// Top-left corner
			if (this->mTopLeftCorner == CornerState::Normal) {
				auto cw = cornerWidth(r, i);
				if (cw > 0) region -= QRegion(x, y + i, cw, 1);
			} else if (this->mTopLeftCorner == CornerState::InvertX) {
				auto cw = cornerWidth(r, i);
				if (cw > 0) region += QRegion(x - cw, y + i, cw, 1);
			} else if (this->mTopLeftCorner == CornerState::InvertY) {
				auto cw = cornerWidth(r, r - i);
				if (cw > 0) region += QRegion(x, y - r + i, cw, 1);
			}

			// Top-right corner
			if (this->mTopRightCorner == CornerState::Normal) {
				auto cw = cornerWidth(r, i);
				if (cw > 0) region -= QRegion(x + w - cw, y + i, cw, 1);
			} else if (this->mTopRightCorner == CornerState::InvertX) {
				auto cw = cornerWidth(r, i);
				if (cw > 0) region += QRegion(x + w, y + i, cw, 1);
			} else if (this->mTopRightCorner == CornerState::InvertY) {
				auto cw = cornerWidth(r, r - i);
				if (cw > 0) region += QRegion(x + w - cw, y - r + i, cw, 1);
			}

			// Bottom-left corner
			if (this->mBottomLeftCorner == CornerState::Normal) {
				auto cw = cornerWidth(r, i);
				if (cw > 0) region -= QRegion(x, y + h - 1 - i, cw, 1);
			} else if (this->mBottomLeftCorner == CornerState::InvertX) {
				auto cw = cornerWidth(r, i);
				if (cw > 0) region += QRegion(x - cw, y + h - 1 - i, cw, 1);
			} else if (this->mBottomLeftCorner == CornerState::InvertY) {
				auto cw = cornerWidth(r, i);
				if (cw > 0) region += QRegion(x, y + h + i, cw, 1);
			}

			// Bottom-right corner
			if (this->mBottomRightCorner == CornerState::Normal) {
				auto cw = cornerWidth(r, i);
				if (cw > 0) region -= QRegion(x + w - cw, y + h - 1 - i, cw, 1);
			} else if (this->mBottomRightCorner == CornerState::InvertX) {
				auto cw = cornerWidth(r, i);
				if (cw > 0) region += QRegion(x + w, y + h - 1 - i, cw, 1);
			} else if (this->mBottomRightCorner == CornerState::InvertY) {
				auto cw = cornerWidth(r, i);
				if (cw > 0) region += QRegion(x + w - cw, y + h + i, cw, 1);
			}
		}
	}

	for (const auto& childRegion: this->mRegions) {
		region = childRegion->applyTo(region);
	}

	return region;
}

QRegion PendingRegion::applyTo(QRegion& region) const {
	switch (this->mIntersection) {
	case Intersection::Combine: region = region.united(this->build()); break;
	case Intersection::Subtract: region = region.subtracted(this->build()); break;
	case Intersection::Intersect: region = region.intersected(this->build()); break;
	case Intersection::Xor: region = region.xored(this->build()); break;
	}

	return region;
}

QRegion PendingRegion::applyTo(const QRect& rect) const {
	// if left as the default, dont combine it with the whole rect area, leave it as is.
	if (this->mIntersection == Intersection::Combine) {
		return this->build();
	} else {
		auto baseRegion = QRegion(rect);
		return this->applyTo(baseRegion);
	}
}

void PendingRegion::regionsAppend(QQmlListProperty<PendingRegion>* prop, PendingRegion* region) {
	auto* self = static_cast<PendingRegion*>(prop->object); // NOLINT
	if (!region) return;

	QObject::connect(region, &QObject::destroyed, self, &PendingRegion::onChildDestroyed);
	QObject::connect(region, &PendingRegion::changed, self, &PendingRegion::childrenChanged);

	self->mRegions.append(region);

	emit self->childrenChanged();
}

PendingRegion* PendingRegion::regionAt(QQmlListProperty<PendingRegion>* prop, qsizetype i) {
	return static_cast<PendingRegion*>(prop->object)->mRegions.at(i); // NOLINT
}

void PendingRegion::regionsClear(QQmlListProperty<PendingRegion>* prop) {
	auto* self = static_cast<PendingRegion*>(prop->object); // NOLINT

	for (auto* region: self->mRegions) {
		QObject::disconnect(region, nullptr, self, nullptr);
	}

	self->mRegions.clear(); // NOLINT
	emit self->childrenChanged();
}

qsizetype PendingRegion::regionsCount(QQmlListProperty<PendingRegion>* prop) {
	return static_cast<PendingRegion*>(prop->object)->mRegions.length(); // NOLINT
}

void PendingRegion::regionsRemoveLast(QQmlListProperty<PendingRegion>* prop) {
	auto* self = static_cast<PendingRegion*>(prop->object); // NOLINT

	auto* last = self->mRegions.last();
	if (last != nullptr) QObject::disconnect(last, nullptr, self, nullptr);

	self->mRegions.removeLast();
	emit self->childrenChanged();
}

void PendingRegion::regionsReplace(
    QQmlListProperty<PendingRegion>* prop,
    qsizetype i,
    PendingRegion* region
) {
	auto* self = static_cast<PendingRegion*>(prop->object); // NOLINT

	auto* old = self->mRegions.at(i);
	if (old != nullptr) QObject::disconnect(old, nullptr, self, nullptr);

	self->mRegions.replace(i, region);
	emit self->childrenChanged();
}
