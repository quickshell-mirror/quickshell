import QtQuick

///! MouseArea wrapper item.
/// This component is useful for wrapping a single component in
/// a mouse area. It works the same as @@WrapperItem, but with a @@QtQuick.MouseArea.
///
/// > [!NOTE] WrapperMouseArea is a @@MarginWrapperManager based component.
/// > See its documentation for information on how margins and sizes are calculated.
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
MouseArea {
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
