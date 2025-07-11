import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Quickshell
import Quickshell.Widgets
import Quickshell.Network

FloatingWindow {
	color: contentItem.palette.window

	ColumnLayout {
		anchors.fill: parent
		anchors.margins: 10
		spacing: 10

		ListView {
			Layout.fillWidth: true
			Layout.fillHeight: true
			model: Network.devices

			delegate: WrapperRectangle {
				width: parent.width
				color: "transparent"
				border.color: palette.button
				border.width: 1
				margin: 10

				ColumnLayout {
					Label { 
						text: `Device ${index}: ${modelData.name}`
						font.bold: true
					}
					Label { text: "Hardware Address: " + modelData.address }
				}
			}
		}
	}
}
