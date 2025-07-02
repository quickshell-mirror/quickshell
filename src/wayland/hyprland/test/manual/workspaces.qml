import QtQuick
import QtQuick.Layouts
import Quickshell
import Quickshell.Widgets
import Quickshell.Hyprland

FloatingWindow {
	ListView {
		anchors.fill: parent
		model: Hyprland.workspaces
		spacing: 5

		delegate: WrapperRectangle {
			id: wsDelegate
			required property HyprlandWorkspace modelData
			color: "lightgray"

			ColumnLayout {
				Text { text: `Workspace ${wsDelegate.modelData.id} on ${wsDelegate.modelData.monitor} | urgent: ${wsDelegate.modelData.urgent}`}

				ColumnLayout {
					Repeater {
						model: wsDelegate.modelData.toplevels
						Text {
							id: tDelegate
							required property HyprlandToplevel modelData;
							text: `${tDelegate.modelData}: ${tDelegate.modelData.title}`
						}
					}
				}
			}
		}
	}
}
