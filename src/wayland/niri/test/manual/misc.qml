import QtQuick
import QtQuick.Layouts
import Quickshell
import Quickshell.Widgets
import Quickshell.Niri

FloatingWindow {
	Connections {
		target: Niri
		function onConfigLoaded(failed: bool) { console.log("niri config loaded, failed:", failed) }
		function onScreenshotCaptured(path: string) { console.log("niri screenshot captured:", path) }
	}

	ColumnLayout {
		anchors.fill: parent

		WrapperRectangle {
			color: Niri.overviewOpen ? "lightblue" : "white"

			Text { text: `overview open: ${Niri.overviewOpen}` }
		}

		WrapperRectangle {
			color: "white"

			Text { text: `keyboard layout: ${Niri.keyboardLayouts[Niri.currentKeyboardLayout] ?? "none"} (${Niri.currentKeyboardLayout + 1}/${Niri.keyboardLayouts.length})` }
		}

		ListView {
			Layout.fillWidth: true
			Layout.fillHeight: true
			model: Niri.casts
			spacing: 5

			delegate: WrapperRectangle {
				id: castDelegate
				required property NiriCast modelData
				color: castDelegate.modelData.active ? "lightgreen" : "white"

				RowLayout {
					Text {
						text: `cast ${castDelegate.modelData.streamId} ${castDelegate.modelData.kind} target: ${castDelegate.modelData.targetOutput || castDelegate.modelData.targetWindowId || "none"} | active: ${castDelegate.modelData.active} dynamic: ${castDelegate.modelData.dynamicTarget} pid: ${castDelegate.modelData.pid} node: ${castDelegate.modelData.pwNodeId}`
					}
				}
			}
		}
	}
}
