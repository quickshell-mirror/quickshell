import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Quickshell
import Quickshell.Widgets
import Quickshell.Network

FloatingWindow {
	color: contentItem.palette.window

	property var defaultDevice: Network.wifi.defaultDevice
	property var sortedNetworks: {
		if (!defaultDevice) return []
		return [...defaultDevice.networks.values].sort((a, b) => {
			if (a.connected !== b.connected) {
				return b.connected - a.connected
			}
			return b.signalStrength - a.signalStrength
		})
	}

	Column {
		anchors.fill: parent
		anchors.margins: 5

		RowLayout { 
			Label {
				text: "WiFi"
				font.bold: true
				font.pointSize: 12
			}
			CheckBox {
				text: "Software"
				checked: Network.wifi.enabled
				onClicked: Network.wifi.setEnabled(!Network.wifi.enabled)
			}
			CheckBox {
				enabled: false
				text: "Hardware"
				checked: Network.wifi.hardwareEnabled
			}
		}

		WrapperRectangle {
			Layout.fillWidth: true
			width: parent.width
			color: "transparent"
			border.color: palette.button
			border.width: 1
			margin: 5
			visible: defaultDevice !== null

			ColumnLayout {
				Label { text: `Default device: ${defaultDevice?.name} (${defaultDevice?.address})` }
				RowLayout {
					Label {
						text: NetworkConnectionState.toString(defaultDevice?.state)
						color: defaultDevice?.state == NetworkConnectionState.Connected ? palette.link : palette.placeholderText
					}
					Label {
						visible: defaultDevice?.state == NetworkConnectionState.Connecting || defaultDevice?.state == NetworkConnectionState.Disconnecting
						text: `(${NMDeviceState.toString(defaultDevice?.nmState)})`
					}
					Button {
						visible: defaultDevice?.state == NetworkConnectionState.Connected
						text: "Disconnect"
						onClicked: defaultDevice?.disconnect()
					}
					Button {
						text: "Scan"
						onClicked: defaultDevice?.scan()
						visible: Network.wifi.defaultDevice?.scanning === false
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
