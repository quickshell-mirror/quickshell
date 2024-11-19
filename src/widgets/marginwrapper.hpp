#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "wrapper.hpp"

namespace qs::widgets {

///! Helper object for applying sizes and margins to a single child item.
/// > [!NOTE] MarginWrapperManager is an extension of @@WrapperManager.
/// > You should read its documentation to understand wrapper types.
///
/// MarginWrapperManager can be used to apply margins to a child item,
/// in addition to handling the size / implicit size relationship
/// between the parent and the child. @@WrapperItem and @@WrapperRectangle
/// exist for Item and Rectangle implementations respectively.
///
/// > [!WARNING] MarginWrapperManager based types set the child item's
/// > @@QtQuick.Item.x, @@QtQuick.Item.y, @@QtQuick.Item.width, @@QtQuick.Item.height
/// > or @@QtQuick.Item.anchors properties. Do not set them yourself,
/// > instead set @@Item.implicitWidth and @@Item.implicitHeight.
///
/// ### Implementing a margin wrapper type
/// Follow the directions in @@WrapperManager$'s documentation, and or
/// alias the @@margin property if you wish to expose it.
class MarginWrapperManager: public WrapperManager {
	Q_OBJECT;
	// clang-format off
	/// The minimum margin between the child item and the parent item's edges.
	/// Defaults to 0.
	Q_PROPERTY(qreal margin READ margin WRITE setMargin NOTIFY marginChanged FINAL);
	/// If the child item should be resized larger than its implicit size if
	/// the parent is resized larger than its implicit size. Defaults to false.
	Q_PROPERTY(bool resizeChild READ resizeChild WRITE setResizeChild NOTIFY resizeChildChanged FINAL);
	// clang-format on
	QML_ELEMENT;

public:
	explicit MarginWrapperManager(QObject* parent = nullptr);

	void componentComplete() override;

	[[nodiscard]] qreal margin() const;
	void setMargin(qreal margin);

	[[nodiscard]] bool resizeChild() const;
	void setResizeChild(bool resizeChild);

signals:
	void marginChanged();
	void resizeChildChanged();

private slots:
	void onChildChanged();
	void onWrapperWidthChanged();
	void onWrapperHeightChanged();
	void onChildImplicitWidthChanged();
	void onChildImplicitHeightChanged();

private:
	void updateGeometry();

	[[nodiscard]] qreal targetChildX() const;
	[[nodiscard]] qreal targetChildY() const;
	[[nodiscard]] qreal targetChildWidth() const;
	[[nodiscard]] qreal targetChildHeight() const;

	qreal mMargin = 0;
	bool mResizeChild = false;
};

} // namespace qs::widgets
