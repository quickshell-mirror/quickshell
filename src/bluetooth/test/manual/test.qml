import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Quickshell
import Quickshell.Widgets
import Quickshell.Bluetooth

FloatingWindow {
	color: contentItem.palette.window

	ListView {
		anchors.fill: parent
		anchors.margins: 5
		model: Bluetooth.adapters

		delegate: WrapperRectangle {
			width: parent.width
			color: "transparent"
			border.color: palette.button
			border.width: 1
			margin: 5

			ColumnLayout {
				Label { text: `Adapter: ${modelData.name} (${modelData.adapterId})` }

				RowLayout {
					Layout.fillWidth: true

					CheckBox {
						text: "Enable"
						checked: modelData.enabled
						onToggled: modelData.enabled = checked
					}

					Label {
						color: modelData.state === BluetoothAdapterState.Blocked ? palette.errorText : palette.placeholderText
						text: BluetoothAdapterState.toString(modelData.state)
					}

					CheckBox {
						text: "Discoverable"
						checked: modelData.discoverable
						onToggled: modelData.discoverable = checked
					}

					CheckBox {
						text: "Discovering"
						checked: modelData.discovering
						onToggled: modelData.discovering = checked
					}

					CheckBox {
						text: "Pairable"
						checked: modelData.pairable
						onToggled: modelData.pairable = checked
					}
				}

				RowLayout {
					Layout.fillWidth: true

					Label { text: "Discoverable timeout:" }

					SpinBox {
						from: 0
						to: 3600
						value: modelData.discoverableTimeout
						onValueModified: modelData.discoverableTimeout = value
						textFromValue: time => time === 0 ? "∞" : time + "s"
					}

					Label { text: "Pairable timeout:" }

					SpinBox {
						from: 0
						to: 3600
						value: modelData.pairableTimeout
						onValueModified: modelData.pairableTimeout = value
						textFromValue: time => time === 0 ? "∞" : time + "s"
					}
				}

				Repeater {
					model: modelData.devices

					WrapperRectangle {
						Layout.fillWidth: true
						color: palette.button
						border.color: palette.mid
						border.width: 1
						margin: 5

						RowLayout {
							ColumnLayout {
								Layout.fillWidth: true

								RowLayout {
									IconImage {
										Layout.fillHeight: true
										implicitWidth: height
										source: Quickshell.iconPath(modelData.icon)
									}

									TextField {
										text: modelData.name
										font.bold: true
										background: null
										readOnly: false
										selectByMouse: true
										onEditingFinished: modelData.name = text
									}

									Label {
										visible: modelData.name && modelData.name !== modelData.deviceName
										text: `(${modelData.deviceName})`
										color: palette.placeholderText
									}
								}

								RowLayout {
									Label {
										text: modelData.address
										color: palette.placeholderText
									}

									Label {
										visible: modelData.batteryAvailable
										text: `| Battery: ${Math.round(modelData.battery * 100)}%`
										color: palette.placeholderText
									}
								}

								RowLayout {
									Label {
										text: BluetoothDeviceState.toString(modelData.state)

										color: modelData.connected ? palette.link : palette.placeholderText
									}

									Label {
										text: modelData.pairing ? "Pairing" : (modelData.paired ? "Paired" : "Not Paired")
										color: modelData.paired || modelData.pairing ? palette.link : palette.placeholderText
									}

									Label {
										visible: modelData.bonded
										text: "| Bonded"
										color: palette.link
									}

									CheckBox {
										text: "Trusted"
										checked: modelData.trusted
										onToggled: modelData.trusted = checked
									}

									CheckBox {
										text: "Blocked"
										checked: modelData.blocked
										onToggled: modelData.blocked = checked
									}

									CheckBox {
										text: "Wake Allowed"
										checked: modelData.wakeAllowed
										onToggled: modelData.wakeAllowed = checked
									}
								}
							}

							ColumnLayout {
								Layout.alignment: Qt.AlignRight

								Button {
									Layout.alignment: Qt.AlignRight
									text: modelData.connected ? "Disconnect" : "Connect"
									onClicked: modelData.connected = !modelData.connected
								}

								Button {
									Layout.alignment: Qt.AlignRight
									text: modelData.pairing ? "Cancel" : (modelData.paired ? "Forget" : "Pair")
									onClicked: {
										if (modelData.pairing) {
											modelData.cancelPair();
										} else if (modelData.paired) {
											modelData.forget();
										} else {
											modelData.pair();
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
}
