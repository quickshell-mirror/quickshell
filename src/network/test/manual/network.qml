import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Quickshell
import Quickshell.Widgets
import Quickshell.Networking

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
					checked: Networking.wifiEnabled
					onClicked: Networking.wifiEnabled = !Networking.wifiEnabled
				}
				CheckBox {
					enabled: false
					text: "Hardware"
					checked: Networking.wifiHardwareEnabled
				}
			}
		}

		ListView {
			clip: true
			Layout.fillWidth: true
			Layout.fillHeight: true
			model: Networking.devices

			delegate: WrapperRectangle {
				width: parent.width
				color: "transparent"
				border.color: palette.button
				border.width: 1
				margin: 5

				ColumnLayout {
					RowLayout {
						Label { text: modelData.name; font.bold: true }
						Label { text: modelData.address }
						Label { text: `(Type: ${DeviceType.toString(modelData.type)})` }
					}
					RowLayout {
						Label {
							text: DeviceConnectionState.toString(modelData.state)
							color: modelData.connected ? palette.link : palette.placeholderText
						}
						Label {
							visible: Networking.backend == NetworkBackendType.NetworkManager && (modelData.state == DeviceConnectionState.Connecting || modelData.state == DeviceConnectionState.Disconnecting)
							text: `(${NMDeviceState.toString(modelData.nmState)})`
						}
						Button {
							visible: modelData.state == DeviceConnectionState.Connected
							text: "Disconnect"
							onClicked: modelData.disconnect()
						}						
						CheckBox {
							text: "Autoconnect"
							checked: modelData.autoconnect
							onClicked: modelData.autoconnect = !modelData.autoconnect
						}
						Label { 
							text: `Mode: ${WifiDeviceMode.toString(modelData.mode)}` 
							visible: modelData.type == DeviceType.Wifi
						}
						CheckBox {
							text: "Scanner"
							checked: modelData.scannerEnabled
							onClicked: modelData.scannerEnabled = !modelData.scannerEnabled
							visible: modelData.type === DeviceType.Wifi
						}
					}

					Repeater {
						Layout.fillWidth: true
						model: {
							if (modelData.type !== DeviceType.Wifi) return []
							return [...modelData.networks.values].sort((a, b) => {
								if (a.connected !== b.connected) {
									return b.connected - a.connected
								}
								return b.signalStrength - a.signalStrength
							})
						}

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
											text: `| Signal strength: ${Math.round(modelData.signalStrength*100)}%`
											color: palette.placeholderText
										}
									}
									Label {
										visible: Networking.backend == NetworkBackendType.NetworkManager && (modelData.nmReason != NMConnectionStateReason.Unknown && modelData.nmReason != NMConnectionStateReason.None)
										text: `Connection change reason: ${NMConnectionStateReason.toString(modelData.nmReason)}`
									}
								}
								RowLayout {
									Layout.alignment: Qt.AlignRight
									Button {
										text: "Connect"
										onClicked: modelData.connect()
										visible: !modelData.connected
									}
									Button {
										text: "Disconnect"
										onClicked: modelData.disconnect()
										visible: modelData.connected
									}
									Button {
										text: "Forget"
										onClicked: modelData.forget()
										visible: modelData.known
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
