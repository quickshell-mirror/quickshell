import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Quickshell
import Quickshell.Wayland

FloatingWindow {
	id: root
	color: contentItem.palette.window

	WaylandWindow.appId: appIdField.text

	ColumnLayout {
		anchors.centerIn: parent

		TextField {
			id: appIdField
			Layout.preferredWidth: 300
			text: "org.quickshell.test"
		}

		Button {
			text: "Hide->Show"
			onClicked: {
				root.visible = false;
				showTimer.start();
			}
		}

		Timer {
			id: showTimer
			interval: 200
			onTriggered: root.visible = true
		}
	}
}
