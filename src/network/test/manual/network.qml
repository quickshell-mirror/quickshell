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
					checked: Network.wifiEnabled
					onClicked: Network.wifiEnabled = !Network.wifiEnabled
				}
				CheckBox {
					enabled: false
					text: "Hardware"
					checked: Network.wifiHardwareEnabled
				}
			}
		}

		ListView {
			clip: true
			Layout.fillWidth: true
			Layout.fillHeight: true
			model: {
				return Network.devices.values.filter(device => device.type === DeviceType.Wifi)
			}

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
					Label { text: `Device: ${modelData.name} (Hardware address: ${modelData.address}) (Type: ${DeviceType.toString(modelData.type)})` }
					RowLayout {
						Label {
							text: DeviceConnectionState.toString(modelData.state)
							color: modelData.state == DeviceConnectionState.Connected ? palette.link : palette.placeholderText
						}
						Label {
							visible: modelData.state == DeviceConnectionState.Connecting || modelData.state == DeviceConnectionState.Disconnecting
							text: `(${NMDeviceState.toString(modelData.nmState)})`
						}
						Button {
							visible: modelData.state == DeviceConnectionState.Connected
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
							color: modelData.connected ? palette.highlight : palette.button
							border.color: palette.mid
							border.width: 1
							margin: 5

							RowLayout {
								ColumnLayout {
									Layout.fillWidth: true
									RowLayout {
										Label { text: modelData.name; font.bold: true }
										Label {
											text: modelData.known ? "Known" : ""
											color: palette.placeholderText
										}
									}
									RowLayout {
										Label { 
											text: `Security: ${WifiSecurityType.toString(modelData.security)}`
											color: palette.placeholderText
										}
										Label {
											text: `| Signal strength: ${modelData.signalStrength*100}%`
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
