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
		anchors.margins: 5

		Column {
			Layout.fillWidth: true
			RowLayout { 
				Label {
					text: "WiFi"
					font.bold: true
					font.pointSize: 12
				}
				CheckBox {
					text: "Software"
					checked: Network.wifi.enabled
					onClicked: Network.wifi.enabled = !Network.wifi.enabled
				}
				CheckBox {
					enabled: false
					text: "Hardware"
					checked: Network.wifi.hardwareEnabled
				}
			}
		}

		ListView {
			Layout.fillWidth: true
			Layout.fillHeight: true
			model: Network.wifi.devices

			delegate: WrapperRectangle {
				width: parent.width
				color: "transparent"
				border.color: palette.button
				border.width: 1
				margin: 5

				property var sortedNetworks: {
					return [...modelData.networks.values].sort((a, b) => {
						if (a.connected !== b.connected) {
							return b.connected - a.connected
						}
						return b.signalStrength - a.signalStrength
					})
				}

				ColumnLayout {
					Label { text: `Device: ${modelData.name} (${modelData.address})` }
					RowLayout {
						Label {
							text: NetworkConnectionState.toString(modelData.state)
							color: modelData.state == NetworkConnectionState.Connected ? palette.link : palette.placeholderText
						}
						Label {
							visible: modelData.state == NetworkConnectionState.Connecting || modelData.state == NetworkConnectionState.Disconnecting
							text: `(${NMDeviceState.toString(modelData.nmState)})`
						}
						Button {
							visible: modelData.state == NetworkConnectionState.Connected
							text: "Disconnect"
							onClicked: modelData.disconnect()
						}
						Button {
							text: "Scan"
							onClicked: modelData.scan()
							visible: modelData.scanning === false
						}
					}

					Repeater {
						Layout.fillWidth: true
						model: sortedNetworks

						WrapperRectangle {
							Layout.fillWidth: true
							color: modelData.connected ? "lightsteelblue" : palette.button
							border.color: palette.mid
							border.width: 1
							margin: 5

							RowLayout {
								ColumnLayout {
									Layout.fillWidth: true
									RowLayout {
										Label { text: modelData.ssid; font.bold: true }
										Label {
											text: modelData.known ? "Known" : ""
											color: palette.placeholderText
										}
									}
									RowLayout {
										Label { 
											text: `Security: ${NMWirelessSecurityType.toString(modelData.nmSecurity)}`
											color: palette.placeholderText
										}
										Label {
											text: `| Signal strength: ${modelData.signalStrength}%`
											color: palette.placeholderText
										}
									}
									Label {
										visible: modelData.nmReason != NMConnectionStateReason.Unknown && modelData.nmReason != NMConnectionStateReason.None
										text: `Connection change reason: ${NMConnectionStateReason.toString(modelData.nmReason)}`
									}
								}
								ColumnLayout {
									Layout.alignment: Qt.AlignRight
									Button {
										Layout.alignment: Qt.AlignRight
										text: "Connect"
										onClicked: modelData.connect()
										visible: !modelData.connected
									}
								}
							}
						}
					}
				}
			}
		}
	}
}
