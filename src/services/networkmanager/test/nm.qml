import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Quickshell
import Quickshell.Widgets
import Quickshell.Services.NetworkManager

FloatingWindow {
	color: contentItem.palette.window

	ColumnLayout {
		anchors.fill: parent
		anchors.margins: 10
		spacing: 10


		ColumnLayout {
			Label { 
				text: "NetworkManager Service Test"
				font.bold: true
			}

			Label { text: "Current state: " + NMState.toString(NetworkManager.state) }
		}

		ListView {
			Layout.fillWidth: true
			Layout.fillHeight: true
			model: NetworkManager.devices

			delegate: WrapperRectangle {
				width: parent.width
				color: "transparent"
				border.color: palette.button
				border.width: 1
				margin: 10

				ColumnLayout {
					Label { 
						text: `Device ${index}: ${modelData.interface}`
						font.bold: true
					}
					Label { text: "Type: " + NMState.toString(modelData.type) }
					Label { text: "State: " + NMState.toString(modelData.state) }
					Label { text: `Managed: ${modelData.managed}` }
				}
			}
		}
	}
}
