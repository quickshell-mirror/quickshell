import QtQuick

///! Image component for displaying widget/icon style images.
/// This is a specialization of @@QtQuick.Image configured for icon-style images,
/// designed to make it easier to use correctly. If you need more control, use
/// @@QtQuick.Image directly.
///
/// The image's aspect raito is assumed to be 1:1. If it is not 1:1, padding
/// will be added to make it 1:1. This is currently applied before the actual
/// aspect ratio of the image is taken into account, and may change in a future
/// release.
///
/// You should use it for:
/// - Icons for custom buttons
/// - Status indicator icons
/// - System tray icons
/// - Things similar to the above.
///
/// Do not use it for:
/// - Big images
/// - Images that change size frequently
/// - Anything that doesn't feel like an icon.
///
/// > [!INFO] More information about many of these properties can be found in
/// > the documentation for @@QtQuick.Image.
Item {
	id: root

	/// URL of the image. Defaults to an empty string.
	/// See @@QtQuick.Image.source.
	property /*string*/alias source: image.source
	/// If the image should be loaded asynchronously. Defaults to false.
	/// See @@QtQuick.Image.asynchronous.
	property /*bool*/alias asynchronous: image.asynchronous
	/// The load status of the image. See @@QtQuick.Image.status.
	property alias status: image.status
	/// If the image should be mipmap filtered. Defaults to false.
	/// See @@QtQuick.Image.mipmap.
	///
	/// Try enabling this if your image is significantly scaled down
	/// and looks bad because of it.
	property /*bool*/alias mipmap: image.mipmap
	/// The @@QtQuick.Image backing this object.
	///
	/// This is useful if you need to access more functionality than
	/// exposed by IconImage.
	property /*Image*/alias backer: image

	/// The suggested size of the image. This is used as a defualt
	/// for @@QtQuick.Item.implicitWidth and @@QtQuick.Item.implicitHeight.
	property real implicitSize: 0

	/// The actual size the image will be displayed at.
	readonly property real actualSize: Math.min(root.width, root.height)

	implicitWidth: root.implicitSize
	implicitHeight: root.implicitSize

	Image {
		id: image
		anchors.fill: parent
		fillMode: Image.PreserveAspectFit

		sourceSize.width: root.actualSize
		sourceSize.height: root.actualSize
	}
}
