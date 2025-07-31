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
						font.pointSize: 12
					}
					Label { text: "Hardware Address: " + modelData.address }
					Label { text: "Device type: " + NetworkDeviceType.toString(modelData.type) }

					RowLayout {
						Label { text: "State: " + NetworkDeviceState.toString(modelData.state) }
						Button {
							text: "Disconnect"
							onClicked: modelData.disconnect()
							visible: modelData.state === NetworkDeviceState.Connected
						}
					}
					ColumnLayout {
						RowLayout {
							Button {
								text: "Scan"
								onClicked: modelData.scan()
								visible: modelData.scanning === false;
							}
						}
						Label { text: "Available networks: " }
						GridLayout {
							columns: 4
							columnSpacing: 30
							rowSpacing: 5

							Label {
								Layout.row: 0
								Layout.column: 0
								text: "SSID"
							}
							Label {
								Layout.row: 0
								Layout.column: 1
								text: "SECURITY"
							}
							Label {
								Layout.row: 0
								Layout.column: 2
								text: "KNOWN"
							}
							Label {
								Layout.row: 0
								Layout.column: 3
								text: "SIGNAL STRENGTH"
							}

							Repeater {
								model: modelData.networks
								delegate: Text {
									Layout.row: index + 1
									Layout.column: 0
									text: modelData.ssid || "[Hidden]"
								}
							}
							Repeater {
								model: modelData.networks
								delegate: Text { 
									Layout.row: index + 1
									Layout.column: 1
									text: NMWirelessSecurityType.toString(modelData.nmSecurity)
								}
							}
							Repeater {
								model: modelData.networks
								delegate: Text { 
									Layout.row: index + 1
									Layout.column: 2
									text: modelData.known ? "*" : ""
								}
							}
							Repeater {
								model: modelData.networks
								delegate: Text { 
									Layout.row: index + 1
									Layout.column: 3
									text: modelData.signalStrength + "%"
								}
							}
						}
						visible: modelData.type === NetworkDeviceType.Wireless
					}
				}
			}
		}
	}
}
