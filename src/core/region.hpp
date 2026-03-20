#pragma once

#include <qobject.h>
#include <qqmlengine.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qquickitem.h>
#include <qregion.h>
#include <qtmetamacros.h>
#include <qtypes.h>

///! Shape of a Region.
/// See @@Region.shape.
namespace RegionShape { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum : quint8 {
	Rect = 0,
	Ellipse = 1,
};
Q_ENUM_NS(Enum);

} // namespace RegionShape

///! Intersection strategy for Regions.
/// See @@Region.intersection.
namespace Intersection { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum : quint8 {
	/// Combine this region, leaving a union of this and the other region. (opposite of `Subtract`)
	Combine = 0,
	/// Subtract this region, cutting this region out of the other. (opposite of `Combine`)
	Subtract = 1,
	/// Create an intersection of this region and the other, leaving only
	/// the area covered by both. (opposite of `Xor`)
	Intersect = 2,
	/// Create an intersection of this region and the other, leaving only
	/// the area not covered by both. (opposite of `Intersect`)
	Xor = 3,
};
Q_ENUM_NS(Enum);

} // namespace Intersection

///! A composable region used as a mask.
/// See @@QsWindow.mask.
class PendingRegion: public QObject {
	Q_OBJECT;
	/// Defaults to `Rect`.
	Q_PROPERTY(RegionShape::Enum shape MEMBER mShape NOTIFY shapeChanged);
	/// The way this region interacts with its parent region. Defaults to `Combine`.
	Q_PROPERTY(Intersection::Enum intersection MEMBER mIntersection NOTIFY intersectionChanged);

	/// The item that determines the geometry of the region.
	/// `item` overrides @@x, @@y, @@width and @@height.
	Q_PROPERTY(QQuickItem* item MEMBER mItem WRITE setItem NOTIFY itemChanged);

	/// Defaults to 0. Does nothing if @@item is set.
	Q_PROPERTY(qint32 x MEMBER mX NOTIFY xChanged);
	/// Defaults to 0. Does nothing if @@item is set.
	Q_PROPERTY(qint32 y MEMBER mY NOTIFY yChanged);
	/// Defaults to 0. Does nothing if @@item is set.
	Q_PROPERTY(qint32 width MEMBER mWidth NOTIFY widthChanged);
	/// Defaults to 0. Does nothing if @@item is set.
	Q_PROPERTY(qint32 height MEMBER mHeight NOTIFY heightChanged);
	// clang-format off
	/// Corner radius for rounded rectangles. Only applies when @@shape is `Rect`. Defaults to 0.
	///
	/// Acts as the default for @@topLeftRadius, @@topRightRadius, @@bottomLeftRadius,
	/// and @@bottomRightRadius.
	Q_PROPERTY(qint32 radius READ radius WRITE setRadius NOTIFY radiusChanged);
	/// Top-left corner radius. Only applies when @@shape is `Rect`.
	///
	/// Defaults to @@radius, and may be reset by assigning `undefined`.
	Q_PROPERTY(qint32 topLeftRadius READ topLeftRadius WRITE setTopLeftRadius RESET resetTopLeftRadius NOTIFY topLeftRadiusChanged);
	/// Top-right corner radius. Only applies when @@shape is `Rect`.
	///
	/// Defaults to @@radius, and may be reset by assigning `undefined`.
	Q_PROPERTY(qint32 topRightRadius READ topRightRadius WRITE setTopRightRadius RESET resetTopRightRadius NOTIFY topRightRadiusChanged);
	/// Bottom-left corner radius. Only applies when @@shape is `Rect`.
	///
	/// Defaults to @@radius, and may be reset by assigning `undefined`.
	Q_PROPERTY(qint32 bottomLeftRadius READ bottomLeftRadius WRITE setBottomLeftRadius RESET resetBottomLeftRadius NOTIFY bottomLeftRadiusChanged);
	/// Bottom-right corner radius. Only applies when @@shape is `Rect`.
	///
	/// Defaults to @@radius, and may be reset by assigning `undefined`.
	Q_PROPERTY(qint32 bottomRightRadius READ bottomRightRadius WRITE setBottomRightRadius RESET resetBottomRightRadius NOTIFY bottomRightRadiusChanged);
	// clang-format on

	/// Regions to apply on top of this region.
	///
	/// Regions can be nested to create a more complex region.
	/// For example this will create a square region with a cutout in the middle.
	/// ```qml
	/// Region {
	///   width: 100; height: 100;
	///
	///   Region {
	///     x: 50; y: 50;
	///     width: 50; height: 50;
	///     intersection: Intersection.Subtract
	///   }
	/// }
	/// ```
	Q_PROPERTY(QQmlListProperty<PendingRegion> regions READ regions);
	Q_CLASSINFO("DefaultProperty", "regions");
	QML_NAMED_ELEMENT(Region);

public:
	explicit PendingRegion(QObject* parent = nullptr);

	void setItem(QQuickItem* item);

	[[nodiscard]] qint32 radius() const;
	void setRadius(qint32 radius);

	[[nodiscard]] qint32 topLeftRadius() const;
	void setTopLeftRadius(qint32 radius);
	void resetTopLeftRadius();

	[[nodiscard]] qint32 topRightRadius() const;
	void setTopRightRadius(qint32 radius);
	void resetTopRightRadius();

	[[nodiscard]] qint32 bottomLeftRadius() const;
	void setBottomLeftRadius(qint32 radius);
	void resetBottomLeftRadius();

	[[nodiscard]] qint32 bottomRightRadius() const;
	void setBottomRightRadius(qint32 radius);
	void resetBottomRightRadius();

	QQmlListProperty<PendingRegion> regions();

	[[nodiscard]] bool empty() const;
	[[nodiscard]] QRegion build() const;
	[[nodiscard]] QRegion applyTo(QRegion& region) const;
	[[nodiscard]] QRegion applyTo(const QRect& rect) const;

	RegionShape::Enum mShape = RegionShape::Rect;
	Intersection::Enum mIntersection = Intersection::Combine;

signals:
	void shapeChanged();
	void intersectionChanged();
	void itemChanged();
	void xChanged();
	void yChanged();
	void widthChanged();
	void heightChanged();
	void radiusChanged();
	void topLeftRadiusChanged();
	void topRightRadiusChanged();
	void bottomLeftRadiusChanged();
	void bottomRightRadiusChanged();
	void childrenChanged();

	/// Triggered when the region's geometry changes.
	///
	/// In some cases the region does not update automatically.
	/// In those cases you can emit this signal manually.
	void changed();

private slots:
	void onItemDestroyed();
	void onChildDestroyed();

private:
	static void regionsAppend(QQmlListProperty<PendingRegion>* prop, PendingRegion* region);
	static PendingRegion* regionAt(QQmlListProperty<PendingRegion>* prop, qsizetype i);
	static void regionsClear(QQmlListProperty<PendingRegion>* prop);
	static qsizetype regionsCount(QQmlListProperty<PendingRegion>* prop);
	static void regionsRemoveLast(QQmlListProperty<PendingRegion>* prop);
	static void
	regionsReplace(QQmlListProperty<PendingRegion>* prop, qsizetype i, PendingRegion* region);

	enum CornerOverride : quint8 {
		TopLeft = 0b1,
		TopRight = 0b10,
		BottomLeft = 0b100,
		BottomRight = 0b1000,
	};

	QQuickItem* mItem = nullptr;

	qint32 mX = 0;
	qint32 mY = 0;
	qint32 mWidth = 0;
	qint32 mHeight = 0;
	qint32 mRadius = 0;
	qint32 mTopLeftRadius = 0;
	qint32 mTopRightRadius = 0;
	qint32 mBottomLeftRadius = 0;
	qint32 mBottomRightRadius = 0;
	quint8 mCornerOverrides = 0;

	QList<PendingRegion*> mRegions;
};
