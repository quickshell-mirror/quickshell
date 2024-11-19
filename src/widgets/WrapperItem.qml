import QtQuick
import Quickshell.Widgets

///! Item that handles sizes and positioning for a single visual child.
/// This component is useful when you need to wrap a single component in
/// an item, or give a single component a margin. See [QtQuick.Layouts]
/// for positioning multiple items.
///
/// > [!NOTE] WrapperItem is a @@MarginWrapperManager based component.
/// > You should read its documentation as well.
///
/// ### Example: Adding a margin to an item
/// The snippet below adds a 10px margin to all sides of the @@QtQuick.Text item.
///
/// ```qml
/// WrapperItem {
///   margin: 10
///
///   @@QtQuick.Text { text: "Hello!" }
/// }
/// ```
///
/// > [!NOTE] The child item can be specified by writing it inline in the wrapper,
/// > as in the example above, or by using the @@child property. See
/// > @@WrapperManager.child for details.
///
/// > [!WARNING] You should not set @@Item.x, @@Item.y, @@Item.width,
/// > @@Item.height or @@Item.anchors on the child item, as they are used
/// > by WrapperItem to position it. Instead set @@Item.implicitWidth and
/// > @@Item.implicitHeight.
///
/// [QtQuick.Layouts]: https://doc.qt.io/qt-6/qtquicklayouts-index.html
Item {
	/// The minimum margin between the child item and the WrapperItem's edges.
	/// Defaults to 0.
	property /*real*/alias margin: manager.margin
	/// If the child item should be resized larger than its implicit size if
	/// the WrapperItem is resized larger than its implicit size. Defaults to false.
	property /*bool*/alias resizeChild: manager.resizeChild
	/// See @@WrapperManager.child for details.
	property /*Item*/alias child: manager.child

	MarginWrapperManager { id: manager }
}
