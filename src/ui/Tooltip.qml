import QtQuick
import Quickshell
import Quickshell.Widgets

PopupWindow {
	id: popup
	required property Item anchorItem
	required property string text
	property string actionText
	property bool show: false

	function showAction() {
		mShowAction = true;
		showInternal = true;
		hangTimer.restart();
	}

	// We should be using a bottom center anchor but support for them is bad compositor side.
	anchor {
		window: anchorItem.QsWindow.window
		adjustment: PopupAdjustment.None
		gravity: Edges.Bottom | Edges.Right

		onAnchoring: {
			const pos = anchorItem.QsWindow.contentItem.mapFromItem(
				anchorItem,
				anchorItem.width / 2 - popup.width / 2,
				anchorItem.height + 5
			);

			anchor.rect.x = pos.x;
			anchor.rect.y = pos.y;
		}
	}

	property bool showInternal: false
	property bool mShowAction: false
	property real opacity: showInternal ? 1 : 0

	onShowChanged: hangTimer.restart()

	Timer {
		id: hangTimer
		interval: 400
		onTriggered: popup.showInternal = popup.show
	}

	Behavior on opacity {
		NumberAnimation {
			duration: 200
			easing.type: popup.showInternal ? Easing.InQuart : Easing.OutQuart
		}
	}

	color: "transparent"
	visible: opacity != 0
	onVisibleChanged: if (!visible) mShowAction = false

	implicitWidth: content.implicitWidth
	implicitHeight: content.implicitHeight

	WrapperRectangle {
		id: content
		opacity: popup.opacity
		color: palette.active.toolTipBase
		border.color: palette.active.light
		margin: 5
		radius: 5

		transform: Scale {
			origin.x: content.width / 2
			origin.y: 0
			xScale: 0.6 + popup.opacity * 0.4
			yScale: xScale
		}

		Text {
			text: popup.mShowAction ? popup.actionText : popup.text
			color: palette.active.toolTipText
		}
	}
}
