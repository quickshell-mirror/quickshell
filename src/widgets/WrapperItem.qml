import QtQuick

///! Item that handles sizes and positioning for a single visual child.
/// This component is useful when you need to wrap a single component in
/// an item, or give a single component a margin. See [QtQuick.Layouts]
/// for positioning multiple items.
///
/// > [!NOTE] WrapperItem is a @@MarginWrapperManager based component.
/// > See its documentation for information on how margins and sizes are calculated.
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
	/// The default for @@topMargin, @@bottomMargin, @@leftMargin and @@rightMargin.
	/// Defaults to 0.
	property /*real*/alias margin: manager.margin
	/// An extra margin applied in addition to @@topMargin, @@bottomMargin,
	/// @@leftMargin, and @@rightMargin. Defaults to 0.
	property /*real*/alias extraMargin: manager.extraMargin
	/// The requested top margin of the content item, not counting @@extraMargin.
	///
	/// Defaults to @@margin, and may be reset by assigning `undefined`.
	property /*real*/alias topMargin: manager.topMargin
	/// The requested bottom margin of the content item, not counting @@extraMargin.
	///
	/// Defaults to @@margin, and may be reset by assigning `undefined`.
	property /*real*/alias bottomMargin: manager.bottomMargin
	/// The requested left margin of the content item, not counting @@extraMargin.
	///
	/// Defaults to @@margin, and may be reset by assigning `undefined`.
	property /*real*/alias leftMargin: manager.leftMargin
	/// The requested right margin of the content item, not counting @@extraMargin.
	///
	/// Defaults to @@margin, and may be reset by assigning `undefined`.
	property /*real*/alias rightMargin: manager.rightMargin
	/// Determines if child item should be resized larger than its implicit size if
	/// the parent is resized larger than its implicit size. Defaults to true.
	property /*bool*/alias resizeChild: manager.resizeChild
	/// Overrides the implicit width of the wrapper.
	///
	/// Defaults to the implicit width of the content item plus its left and right margin,
	/// and may be reset by assigning `undefined`.
	property /*real*/alias implicitWidth: manager.implicitWidth
	/// Overrides the implicit height of the wrapper.
	///
	/// Defaults to the implicit width of the content item plus its top and bottom margin,
	/// and may be reset by assigning `undefined`.
	property /*real*/alias implicitHeight: manager.implicitHeight
	/// See @@WrapperManager.child for details.
	property /*Item*/alias child: manager.child

	MarginWrapperManager { id: manager }
}
