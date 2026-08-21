import QtQuick
import QtQuick.Layouts
import Quickshell
import Quickshell.Widgets
import Quickshell.Niri

FloatingWindow {
	ColumnLayout {
		anchors.fill: parent

		ListView {
			Layout.fillWidth: true
			Layout.fillHeight: true
			model: Niri.monitors
			spacing: 5

			delegate: WrapperRectangle {
				id: monDelegate
				required property NiriMonitor modelData
				color: monDelegate.modelData.focused ? "lightblue" : "white"

				RowLayout {
					Text {
						text: `${monDelegate.modelData.name} "${monDelegate.modelData.make} ${monDelegate.modelData.model}" ${monDelegate.modelData.width}x${monDelegate.modelData.height}@${monDelegate.modelData.scale} pos ${monDelegate.modelData.x},${monDelegate.modelData.y} | focused: ${monDelegate.modelData.focused} activeWorkspace: ${monDelegate.modelData.activeWorkspace?.idx ?? "none"}`
					}
				}
			}
		}

		// Niri sends no output events, so refresh must be triggered manually.
		WrapperRectangle {
			color: "lightyellow"

			Text {
				text: "Refresh monitors"

				MouseArea {
					anchors.fill: parent
					onClicked: Niri.refreshMonitors()
				}
			}
		}

		// Verify monitorFor against quickshell screens.
		Repeater {
			model: Quickshell.screens

			delegate: Text {
				id: screenDelegate
				required property ShellScreen modelData
				text: `screen ${screenDelegate.modelData.name} -> niri monitor ${Niri.monitorFor(screenDelegate.modelData)?.name ?? "none"}`
			}
		}
	}
}
