import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Quickshell
import Quickshell.Wayland

Scope {
	Timer {
		id: toggleTimer
		interval: 100
		onTriggered: windowLoader.active = true
	}

	LazyLoader {
		id: windowLoader
		active: true

		property bool enabled: false

		FloatingWindow {
			id: w
			color: contentItem.palette.window

			ColumnLayout {
				anchors.centerIn: parent

				CheckBox {
					id: loadedCb
					text: "Loaded"
					checked: true
				}

				CheckBox {
					id: enabledCb
					text: "Enabled"
					checked: windowLoader.enabled
					onCheckedChanged: windowLoader.enabled = checked
				}

				Label {
					text: `Active: ${inhibitorLoader.item?.active ?? false}`
				}

				Button {
					text: "Toggle Window"
					onClicked: {
						windowLoader.active = false;
						toggleTimer.start();
					}
				}
			}

			LazyLoader {
				id: inhibitorLoader
				active: loadedCb.checked

				ShortcutInhibitor {
					window: w
					enabled: enabledCb.checked
					onCancelled: enabledCb.checked = false
				}
			}
		}
	}
}
