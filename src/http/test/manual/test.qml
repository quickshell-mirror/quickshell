import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Quickshell
import Quickshell.Http

FloatingWindow {
	minimumSize: "1000x1000" 
	ColumnLayout {
		anchors {
			fill: parent
			margins: 20
		}
		spacing: 20

		ColumnLayout {
			spacing: 5
			Layout.alignment: Qt.AlignTop
			Layout.fillWidth: true
			Label { text: "URL" }
			TextField {
				id: urlField
				Layout.fillWidth: true
				selectByMouse: true
				text: "https://example.com"
			}
			Label { text: "Timeout"}
			TextField {
				id: timeoutField
				Layout.fillWidth: true
				text: "1000"
				validator: IntValidator { bottom: 0 }
				inputMethodHints: Qt.ImhDigitsOnly
			}
			Label { text: "Method" }
			ComboBox {
				id: methodCombo
				Layout.fillWidth: true
				model: ["GET", "POST", "PUT", "DELETE", "HEAD"]
			}
			Label { text: "Response text" }
			ScrollView {
				Layout.fillWidth: true
				Layout.fillHeight: true

				TextArea {
					id: respArea
					readOnly: true
					wrapMode: TextEdit.Wrap
				}
			}

			RowLayout {
				id: buttonRow
				Layout.fillWidth: true
				Layout.topMargin: 10
				Layout.alignment: Qt.AlignRight
				spacing: 10

				function sendRequest(wTimeout: bool) {
					respArea.text = "Sending request...";
					let opts = {
						headers: {
							"User-Agent": "Quickshell"
						}
					};
					if (wTimeout) {
						opts.timeout = Number(timeoutField.text)
					}

					HttpClient.request(urlField.text, methodCombo.currentText, opts, resp => {
						if (resp.success()) {
							setRespText(resp.text());
						} else {
							setRespText(resp.timeouted ? "Timeout error" : `Error: ${reps.statusText}`);
						}
					});
				}

				// workaround for the TextArea id
				function setRespText(text: string) {
					respArea.text = text;
				}
				
				Button {
					text: "Fetch with timeout"
					onClicked: {
						parent.sendRequest(true);
					}
				}
				Button {
					text: "Fetch"
					onClicked: {
						parent.sendRequest(false);
					}
				}
			}
		}
	}
}
