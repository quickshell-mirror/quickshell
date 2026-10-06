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
	QObject::connect(this, &PendingRegion::topLeftRadiusChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::topRightRadiusChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::bottomLeftRadiusChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::bottomRightRadiusChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::childrenChanged, this, &PendingRegion::changed);
}

void PendingRegion::setItem(QQuickItem* item) {
	if (item == this->mItem) return;

	if (this->mItem != nullptr) {
		QObject::disconnect(this->mItem, nullptr, this, nullptr);
		this->disconnectParentChain();
	}

	this->mItem = item;

	if (item != nullptr) {
		QObject::connect(this->mItem, &QObject::destroyed, this, &PendingRegion::onItemDestroyed);
		QObject::connect(this->mItem, &QQuickItem::xChanged, this, &PendingRegion::itemChanged);
		QObject::connect(this->mItem, &QQuickItem::yChanged, this, &PendingRegion::itemChanged);
		QObject::connect(this->mItem, &QQuickItem::widthChanged, this, &PendingRegion::itemChanged);
		QObject::connect(this->mItem, &QQuickItem::heightChanged, this, &PendingRegion::itemChanged);
		this->connectParentChain(item);
	}

	emit this->itemChanged();
}

void PendingRegion::onItemDestroyed() {
	this->mItem = nullptr;
	this->disconnectParentChain();
}

void PendingRegion::onChildDestroyed() { this->mRegions.removeAll(this->sender()); }

void PendingRegion::connectParentChain(QQuickItem* item) {
	this->disconnectParentChain();

	auto* parent = item->parentItem();
	while (parent != nullptr) {
		this->mParentConnections.push_back(
		    QObject::connect(parent, &QQuickItem::xChanged, this, &PendingRegion::itemChanged)
		);
		this->mParentConnections.push_back(
		    QObject::connect(parent, &QQuickItem::yChanged, this, &PendingRegion::itemChanged)
		);
		parent = parent->parentItem();
	}
}

void PendingRegion::disconnectParentChain() {
	for (const auto& connection: this->mParentConnections) {
		QObject::disconnect(connection);
	}
	this->mParentConnections.clear();
}

qint32 PendingRegion::radius() const { return this->mRadius; }

void PendingRegion::setRadius(qint32 radius) {
	if (radius == this->mRadius) return;
	this->mRadius = radius;
	emit this->radiusChanged();

	if (!(this->mCornerOverrides & TopLeft)) emit this->topLeftRadiusChanged();
	if (!(this->mCornerOverrides & TopRight)) emit this->topRightRadiusChanged();
	if (!(this->mCornerOverrides & BottomLeft)) emit this->bottomLeftRadiusChanged();
	if (!(this->mCornerOverrides & BottomRight)) emit this->bottomRightRadiusChanged();
}

qint32 PendingRegion::topLeftRadius() const {
	return (this->mCornerOverrides & TopLeft) ? this->mTopLeftRadius : this->mRadius;
}

void PendingRegion::setTopLeftRadius(qint32 radius) {
	this->mTopLeftRadius = radius;
	this->mCornerOverrides |= TopLeft;
	emit this->topLeftRadiusChanged();
}

void PendingRegion::resetTopLeftRadius() {
	this->mCornerOverrides &= ~TopLeft;
	emit this->topLeftRadiusChanged();
}

qint32 PendingRegion::topRightRadius() const {
	return (this->mCornerOverrides & TopRight) ? this->mTopRightRadius : this->mRadius;
}

void PendingRegion::setTopRightRadius(qint32 radius) {
	this->mTopRightRadius = radius;
	this->mCornerOverrides |= TopRight;
	emit this->topRightRadiusChanged();
}

void PendingRegion::resetTopRightRadius() {
	this->mCornerOverrides &= ~TopRight;
	emit this->topRightRadiusChanged();
}

qint32 PendingRegion::bottomLeftRadius() const {
	return (this->mCornerOverrides & BottomLeft) ? this->mBottomLeftRadius : this->mRadius;
}

void PendingRegion::setBottomLeftRadius(qint32 radius) {
	this->mBottomLeftRadius = radius;
	this->mCornerOverrides |= BottomLeft;
	emit this->bottomLeftRadiusChanged();
}

void PendingRegion::resetBottomLeftRadius() {
	this->mCornerOverrides &= ~BottomLeft;
	emit this->bottomLeftRadiusChanged();
}

qint32 PendingRegion::bottomRightRadius() const {
	return (this->mCornerOverrides & BottomRight) ? this->mBottomRightRadius : this->mRadius;
}

void PendingRegion::setBottomRightRadius(qint32 radius) {
	this->mBottomRightRadius = radius;
	this->mCornerOverrides |= BottomRight;
	emit this->bottomRightRadiusChanged();
}

void PendingRegion::resetBottomRightRadius() {
	this->mCornerOverrides &= ~BottomRight;
	emit this->bottomRightRadiusChanged();
}

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

	if (this->mShape == RegionShape::Rect && !region.isEmpty()) {
		auto tl = std::max(this->topLeftRadius(), 0);
		auto tr = std::max(this->topRightRadius(), 0);
		auto bl = std::max(this->bottomLeftRadius(), 0);
		auto br = std::max(this->bottomRightRadius(), 0);

		if (tl > 0 || tr > 0 || bl > 0 || br > 0) {
			auto rect = region.boundingRect();
			auto x = rect.x();
			auto y = rect.y();
			auto w = rect.width();
			auto h = rect.height();

			// Normalize so adjacent corners don't exceed their shared edge.
			// Each corner is scaled by the tightest constraint of its two edges.
			auto topScale = tl + tr > w ? static_cast<double>(w) / (tl + tr) : 1.0;
			auto bottomScale = bl + br > w ? static_cast<double>(w) / (bl + br) : 1.0;
			auto leftScale = tl + bl > h ? static_cast<double>(h) / (tl + bl) : 1.0;
			auto rightScale = tr + br > h ? static_cast<double>(h) / (tr + br) : 1.0;

			tl = static_cast<qint32>(tl * std::min(topScale, leftScale));
			tr = static_cast<qint32>(tr * std::min(topScale, rightScale));
			bl = static_cast<qint32>(bl * std::min(bottomScale, leftScale));
			br = static_cast<qint32>(br * std::min(bottomScale, rightScale));

			// Unlock each corner: subtract (cornerBox - quarterEllipse) from the
			// full rect. Each corner only modifies pixels inside its own box,
			// so no diagonal overlap is possible.
			if (tl > 0) {
				auto box = QRegion(x, y, tl, tl);
				auto ellipse = QRegion(x, y, tl * 2, tl * 2, QRegion::Ellipse);
				region -= box - (ellipse & box);
			}

			if (tr > 0) {
				auto box = QRegion(x + w - tr, y, tr, tr);
				auto ellipse = QRegion(x + w - tr * 2, y, tr * 2, tr * 2, QRegion::Ellipse);
				region -= box - (ellipse & box);
			}

			if (bl > 0) {
				auto box = QRegion(x, y + h - bl, bl, bl);
				auto ellipse = QRegion(x, y + h - bl * 2, bl * 2, bl * 2, QRegion::Ellipse);
				region -= box - (ellipse & box);
			}

			if (br > 0) {
				auto box = QRegion(x + w - br, y + h - br, br, br);
				auto ellipse = QRegion(x + w - br * 2, y + h - br * 2, br * 2, br * 2, QRegion::Ellipse);
				region -= box - (ellipse & box);
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
	// if left as the default, don't combine it with the whole rect area, leave it as is.
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
