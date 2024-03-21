#include "region.hpp"
#include <cmath>

#include <qobject.h>
#include <qpoint.h>
#include <qqmllist.h>
#include <qquickitem.h>
#include <qregion.h>
#include <qtmetamacros.h>

PendingRegion::PendingRegion(QObject* parent): QObject(parent) {
	QObject::connect(this, &PendingRegion::shapeChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::intersectionChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::itemChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::xChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::yChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::widthChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::heightChanged, this, &PendingRegion::changed);
	QObject::connect(this, &PendingRegion::childrenChanged, this, &PendingRegion::changed);
}

void PendingRegion::setItem(QQuickItem* item) {
	if (this->mItem != nullptr) {
		QObject::disconnect(this->mItem, nullptr, this, nullptr);
	}

	this->mItem = item;

	if (item != nullptr) {
		QObject::connect(this->mItem, &QQuickItem::xChanged, this, &PendingRegion::itemChanged);
		QObject::connect(this->mItem, &QQuickItem::yChanged, this, &PendingRegion::itemChanged);
		QObject::connect(this->mItem, &QQuickItem::widthChanged, this, &PendingRegion::itemChanged);
		QObject::connect(this->mItem, &QQuickItem::heightChanged, this, &PendingRegion::itemChanged);
	}

	emit this->itemChanged();
}

void PendingRegion::onItemDestroyed() { this->mItem = nullptr; }

QQmlListProperty<PendingRegion> PendingRegion::regions() {
	return QQmlListProperty<PendingRegion>(
	    this,
	    nullptr,
	    PendingRegion::regionsAppend,
	    nullptr,
	    nullptr,
	    nullptr,
	    nullptr,
	    nullptr
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

void PendingRegion::regionsAppend(QQmlListProperty<PendingRegion>* prop, PendingRegion* region) {
	auto* self = static_cast<PendingRegion*>(prop->object); // NOLINT
	region->setParent(self);
	self->mRegions.append(region);

	QObject::connect(region, &PendingRegion::changed, self, &PendingRegion::childrenChanged);
	emit self->childrenChanged();
}
