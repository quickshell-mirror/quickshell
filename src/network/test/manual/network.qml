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
						Label {
							text: modelData.name
							font.bold: true
						}
						Label {
							text: modelData.address
						}
						Label {
							text: `(Type: ${DeviceType.toString(modelData.type)})`
						}
						CheckBox {
							text: `Managed`
							checked: modelData.nmManaged
							onClicked: modelData.nmManaged = !modelData.nmManaged
						}
					}
					RowLayout {
						Label {
							text: DeviceConnectionState.toString(modelData.state)
							color: modelData.connected ? palette.link : palette.placeholderText
						}
						Label {
							text: `(${NMDeviceState.toString(modelData.nmState)}: ${NMDeviceStateReason.toString(modelData.nmStateReason)})`
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
						model: ScriptModel {
							values: [...modelData.networks.values].sort((a, b) => {
								if (a.connected !== b.connected) {
									return b.connected - a.connected;
								}
								return b.signalStrength - a.signalStrength;
							})
						}

						WrapperRectangle {
							property var context: NMConnectionContext {
								network: modelData
								onActivating: contextLoader.sourceComponent = null
								onSettingsChanged: contextLoader.sourceComponent = null

								onNoSecrets: {
									contextLoader.sourceComponent = secretsComponent;
								}
							}

							Component {
								id: secretsComponent
								RowLayout {
									Label { text: "Secrets required!" }
									Label { text: "PSK:"; visible: pskField.visible }
									TextField {
										id: pskField
										placeholderText: "Password"
										visible: modelData.security === WifiSecurityType.WpaPsk || modelData.security === WifiSecurityType.Wpa2Psk || modelData.security === WifiSecurityType.Sae
									}
									Label { text: "WEP Key:"; visible: wepKeyField.visible }
									TextField {
										id: wepKeyField
										placeholderText: "Key"
										visible: modelData.security === WifiSecurityType.StaticWep
									}
									Label { text: "Index:"; visible: wepKeyIndex.visible }
									SpinBox {
										id: wepKeyIndex
										from: 0
										to: 3
										value: 0
										visible: wepKeyField.visible
									}
									Label { text: "Type:"; visible: wepKeyType.visible }
									ComboBox {
										id: wepKeyType
										model: ["Key", "Passphrase"]
										visible: wepKeyField.visible
									}
									Button {
										text: "Set"
										visible: pskField.visible || wepKeyField.visible
										onClicked: {
											if (pskField.visible) {
												context.settings?.wirelessSecuritySettings?.setPsk(pskField.text);
											} else if (wepKeyField.visible) {
												context.settings?.wirelessSecuritySettings?.setWepKey(wepKeyField.text, wepKeyIndex.value, wepKeyType.currentIndex + 1);
											}
											contextLoader.sourceComponent = null;
										}
									}
									Button {
										text: "Cancel"
										onClicked: contextLoader.sourceComponent = null
									}
								}
							}

							Layout.fillWidth: true
							color: modelData.connected ? palette.highlight : palette.button
							border.color: palette.mid
							border.width: 1
							margin: 5

							RowLayout {
								ColumnLayout {
									Layout.fillWidth: true
									RowLayout {
										Label {
											text: modelData.name
											font.bold: true
										}
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
											text: `| Signal strength: ${Math.round(modelData.signalStrength * 100)}%`
											color: palette.placeholderText
										}
									}
								}
								ColumnLayout {
									Layout.alignment: Qt.AlignRight
									RowLayout {
										Layout.alignment: Qt.AlignRight
										BusyIndicator {
											implicitHeight: 30
											implicitWidth: 30
											running: modelData.stateChanging
											visible: modelData.stateChanging
										}
										Label {
											text: NetworkState.toString(modelData.state)
											color: modelData.connected ? palette.link : palette.placeholderText
										}
										Label {
											visible: modelData.nmStateReason != NMNetworkStateReason.None && modelData.nmStateReason != NMNetworkStateReason.Unknown
											text: `(${NMNetworkStateReason.toString(modelData.stateReason)})`
											color: palette.placeholderText
										}
										RowLayout {
											id: settingsConnectionRow
											property var selectedSettings: modelData.nmSettings.values[0]
											Label {
												text: "Choose settings:"
											}
											ComboBox {
												id: settingsComboBox
												model: modelData.nmSettings.values.map(s => s?.connectionSettings?.id)
												currentIndex: 0
												onActivated: function (index) {
													settingsConnectionRow.selectedSettings = modelData.nmSettings.values[index];
												}
											}
											Button {
												text: "Connect"
												onClicked: modelData.connectWithSettings(settingsConnectionRow.selectedSettings)
											}
											visible: !modelData.connected && modelData.nmSettings.values.length > 1
										}
										Button {
											text: "Connect"
											onClicked: modelData.connect()
											visible: !modelData.connected && modelData.nmSettings.values.length <= 1
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
									Loader {
										id: contextLoader
										Layout.alignment: Qt.AlignRight
										visible: sourceComponent !== null
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
