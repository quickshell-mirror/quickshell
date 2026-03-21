import QtQuick
import QtQuick.Controls
import Quickshell
import Quickshell.Wayland

FloatingWindow {
	id: root
	color: contentItem.palette.window

	CheckBox {
		id: enableCb
		anchors.centerIn: parent
		text: "Enable Inhibitor"
	}

	IdleInhibitor {
		window: root
		enabled: enableCb.checked
	}
}
