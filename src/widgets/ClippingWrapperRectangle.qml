import QtQuick

///! ClippingRectangle that handles sizes and positioning for a single visual child.
/// This component is useful for adding a clipping border or background rectangle to
/// a child item. If you don't need clipping, use @@WrapperRectangle.
///
/// > [!NOTE] ClippingWrapperRectangle is a @@MarginWrapperManager based component.
/// > You should read its documentation as well.
///
/// > [!WARNING] You should not set @@Item.x, @@Item.y, @@Item.width,
/// > @@Item.height or @@Item.anchors on the child item, as they are used
/// > by WrapperItem to position it. Instead set @@Item.implicitWidth and
/// > @@Item.implicitHeight.
ClippingRectangle {
	id: root

	/// The minimum margin between the child item and the ClippingWrapperRectangle's
	/// edges. Defaults to 0.
	property /*real*/alias margin: manager.margin
	/// If the child item should be resized larger than its implicit size if
	/// the WrapperRectangle is resized larger than its implicit size. Defaults to false.
	property /*bool*/alias resizeChild: manager.resizeChild
	/// See @@WrapperManager.child for details.
	property alias child: manager.child

	implicitWidth: root.contentItem.implicitWidth + (root.contentInsideBorder ? root.border.width * 2 : 0)
	implicitHeight: root.contentItem.implicitHeight + (root.contentInsideBorder ? root.border.width * 2 : 0)

	resources: [
		MarginWrapperManager {
			id: manager
			wrapper: root.contentItem
		}
	]
}
