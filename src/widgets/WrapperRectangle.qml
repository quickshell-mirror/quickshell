import QtQuick

///! Rectangle that handles sizes and positioning for a single visual child.
/// This component is useful for adding a border or background rectangle to
/// a child item. If you need to clip the child item to the rectangle's
/// border, see @@ClippingWrapperRectangle.
///
/// > [!NOTE] WrapperRectangle is a @@MarginWrapperManager based component.
/// > You should read its documentation as well.
///
/// > [!WARNING] You should not set @@Item.x, @@Item.y, @@Item.width,
/// > @@Item.height or @@Item.anchors on the child item, as they are used
/// > by WrapperItem to position it. Instead set @@Item.implicitWidth and
/// > @@Item.implicitHeight.
Rectangle {
	id: root

	/// If true (default), the rectangle's border width will be added
	/// to the margin.
	property bool contentInsideBorder: true
	/// The minimum margin between the child item and the WrapperRectangle's
	/// edges. If @@contentInsideBorder is true, this excludes the border,
	/// otherwise it includes it. Defaults to 0.
	property real margin: 0
	/// If the child item should be resized larger than its implicit size if
	/// the WrapperRectangle is resized larger than its implicit size. Defaults to false.
	property /*bool*/alias resizeChild: manager.resizeChild
	/// See @@WrapperManager.child for details.
	property alias child: manager.child

	MarginWrapperManager {
		id: manager
		margin: (root.contentInsideBorder ? root.border.width : 0) + root.margin
	}
}
