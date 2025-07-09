import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Quickshell
import Quickshell.Widgets
import Quickshell.Services.NetworkManager

FloatingWindow {
	color: contentItem.palette.window
	ListView {
		anchors.fill: parent
		anchors.margins: 5
		model: NetworkManager.devices
		delegate: WrapperRectangle {
			width: parent.width
			color: "transparent"
			border.color: palette.button
			border.width: 1
			margin: 5

			ColumnLayout {
				Label { 
					text: `Device ${index}: ${modelData.interface}`
					font.bold: true
				}
				Label { text: "Type: " + NetworkManagerDeviceType.toString(modelData.type) }
				Label { text: "State: " + NetworkManagerDeviceState.toString(modelData.state) }
				Label { text: `Managed: ${modelData.managed}` }
			}
		}
	}
}
