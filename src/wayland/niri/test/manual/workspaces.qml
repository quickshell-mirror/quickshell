import QtQuick
import QtQuick.Layouts
import Quickshell
import Quickshell.Widgets
import Quickshell.Niri

FloatingWindow {
	ListView {
		anchors.fill: parent
		model: Niri.workspaces
		spacing: 5

		delegate: WrapperRectangle {
			id: wsDelegate
			required property NiriWorkspace modelData
			color: wsDelegate.modelData.focused ? "lightblue" : wsDelegate.modelData.active ? "lightgray" : "white"

			RowLayout {
				Text {
					text: `Workspace ${wsDelegate.modelData.idx} "${wsDelegate.modelData.name}" (id ${wsDelegate.modelData.id}) on ${wsDelegate.modelData.monitor?.name ?? "none"} | active: ${wsDelegate.modelData.active} focused: ${wsDelegate.modelData.focused} urgent: ${wsDelegate.modelData.urgent}`

					MouseArea {
						anchors.fill: parent
						onClicked: {
							console.log("clicked workspace id", wsDelegate.modelData.id)
							wsDelegate.modelData.activate()
						}
					}
				}
			}
		}
	}
}
