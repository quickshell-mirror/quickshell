import Quickshell
import Quickshell.Services.Polkit
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Scope {
	id: root

	FloatingWindow {
		title: "Authentication Required"

		visible: polkitAgent.isActive
		color: contentItem.palette.window

		ColumnLayout {
			id: contentColumn
			anchors.fill: parent
			anchors.margins: 18
			spacing: 12

			Item { Layout.fillHeight: true }

			Label {
				Layout.fillWidth: true
				text: polkitAgent.flow?.message || "<no message>"
				wrapMode: Text.Wrap
				font.bold: true
			}

			Label {
				Layout.fillWidth: true
				text: polkitAgent.flow?.supplementaryMessage || "<no supplementary message>"
				wrapMode: Text.Wrap
				opacity: 0.8
			}

			Label {
				Layout.fillWidth: true
				text: polkitAgent.flow?.inputPrompt || "<no input prompt>"
				wrapMode: Text.Wrap
			}

			Label {
				text: "Authentication failed, try again"
				color: "red"
				visible: polkitAgent.flow?.failed
			}

			TextField {
				id: passwordInput
				echoMode: polkitAgent.flow?.responseVisible
									? TextInput.Normal : TextInput.Password
				selectByMouse: true
				Layout.fillWidth: true
				onAccepted: okButton.clicked()
			}

			RowLayout {
				spacing: 8
				Button {
					id: okButton
					text: "OK"
					enabled: passwordInput.text.length > 0 || !!polkitAgent.flow?.isResponseRequired
					onClicked: {
						polkitAgent.flow.submit(passwordInput.text)
						passwordInput.text = ""
						passwordInput.forceActiveFocus()
					}
				}
				Button {
					text: "Cancel"
					visible: polkitAgent.isActive
					onClicked: {
						polkitAgent.flow.cancelAuthenticationRequest()
						passwordInput.text = ""
					}
				}
			}

			Item { Layout.fillHeight: true }
		}

		Connections {
			target: polkitAgent.flow
			function onIsResponseRequiredChanged() {
				passwordInput.text = ""
				if (polkitAgent.flow.isResponseRequired)
					passwordInput.forceActiveFocus()
			}
		}
	}

	PolkitAgent {
		id: polkitAgent
	}
}
