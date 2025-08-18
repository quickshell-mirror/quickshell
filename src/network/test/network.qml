import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Quickshell
import Quickshell.Widgets
import Quickshell.Network

FloatingWindow {
	property var sortedNetworks: {
		return [...Network.wifi.defaultDevice.networks.values].sort((a, b) => {
			const aIsConnected = a.state === NetworkConnectionState.Connected
			const bIsConnected = b.state === NetworkConnectionState.Connected

			if (aIsConnected !== bIsConnected) {
				return bIsConnected - aIsConnected
			}
			return b.signalStrength - a.signalStrength
		})
	}
	Column {
		anchors.fill: parent
		anchors.margins: 10
		spacing: 10

		// WiFi toggle
		RowLayout { 
			Label {
				text: "WiFi"
				font.bold: true
				font.pointSize: 16
			}
			Switch {
				checked: Network.wifi.enabled
				onClicked: Network.wifi.setEnabled(!Network.wifi.enabled)
			}
		}

		Label {
			text: "Devices"
			font.bold: true
			font.pointSize: 12
		}

		WrapperRectangle {
			width: 175
			implicitHeight: deviceList.contentHeight

			Component {
				id: deviceDelegate
				Item {
					id: deviceItem
					implicitWidth: deviceRow.implicitWidth + 10
					height: 20
					property bool isDefault: Network.wifi.defaultDevice === modelData
					MouseArea {
						anchors.fill: parent
						onClicked: {
							Network.wifi.defaultDevice = modelData
						}
					}
					RowLayout {
						id: deviceRow
						anchors.centerIn: parent
						spacing: 5
						Text { 
							text: modelData.name; font.bold: true; 
							Layout.preferredWidth: 60
							Layout.alignment: Qt.AlignLeft
							elide: Text.ElideRight
						}
						Text { 
							text: modelData.address
							Layout.preferredWidth: 115
							Layout.alignment: Qt.AlignLeft
						}
					}
				}
			}

			ListView {
				id: deviceList
				model: Network.wifi.devices
				interactive: false
				delegate: deviceDelegate
				highlight: Rectangle { color: "lightskyblue"; radius: 5; border.width: 0; border.color: "grey" }
				currentIndex: model.indexOf(Network.wifi.defaultDevice)
			}
		}

		RowLayout { 
			Label {
				text: "Networks"
				font.bold: true
				font.pointSize: 12
			}
			Button {
				Layout.preferredWidth: 42
				Layout.preferredHeight: 20
				text: "Scan"
				font.pointSize: 10
				onClicked: Network.wifi.defaultDevice.scan()
				visible: Network.wifi.defaultDevice.scanning === false;
			}
		}

		WrapperRectangle {
			width: 375
			implicitHeight: networkList.contentHeight

			Component {
				id: networkDelegate
				RowLayout {
					Rectangle {
						id: networkItem
						implicitWidth: networkRow.implicitWidth + 10
						height: 20
						color: modelData.state === NetworkConnectionState.Connected ? "lightskyblue" : "whitesmoke"; 
						radius: 4; border.width: 0; border.color: "grey"
						MouseArea {
							anchors.fill: parent
							hoverEnabled: true
							onEntered: { parent.color = "silver" }
							onExited: { parent.color = modelData.state === NetworkConnectionState.Connected ? "lightskyblue" : "whitesmoke" }
							onClicked: modelData.connect()
							visible: modelData.state !== NetworkConnectionState.Connected;
						}
						RowLayout {
							id: networkRow
							anchors.centerIn: parent
							spacing: 8
							Text { 
								text: modelData.ssid
								Layout.preferredWidth: 100
								horizontalAlignment: Text.AlignLeft
								elide: Text.ElideRight
							}
							Text { 
								text: NMWirelessSecurityType.toString(modelData.nmSecurity)
								Layout.preferredWidth: 80
								horizontalAlignment: Text.AlignRight
								elide: Text.ElideLeft
							}
							Text { 
								text: modelData.known ? "KNOWN" : ""
								Layout.preferredWidth: 50
								horizontalAlignment: Text.AlignLeft
								elide: Text.ElideRight
							}
							Text {
								text: modelData.signalStrength + "%"	
								Layout.preferredWidth: 25
								horizontalAlignment: Text.AlignLeft
							}
						}
					}
				}
			}
			ListView {
				id: networkList
				model: sortedNetworks
				interactive: false
				delegate: networkDelegate
				spacing: 2
			}
		}
	}
}
