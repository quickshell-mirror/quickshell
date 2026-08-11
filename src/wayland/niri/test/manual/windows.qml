import QtQuick
import QtQuick.Layouts
import Quickshell
import Quickshell.Widgets
import Quickshell.Niri

FloatingWindow {
	ListView {
		anchors.fill: parent
		model: Niri.windows
		spacing: 5

		delegate: WrapperRectangle {
			id: winDelegate
			required property NiriWindow modelData
			color: winDelegate.modelData.focused ? "lightblue" : winDelegate.modelData.urgent ? "orange" : "white"

			RowLayout {
				Text {
					text: `${winDelegate.modelData.appId} "${winDelegate.modelData.title}" (id ${winDelegate.modelData.id}) ws ${winDelegate.modelData.workspace?.idx ?? "none"} | floating: ${winDelegate.modelData.floating} focused: ${winDelegate.modelData.focused} urgent: ${winDelegate.modelData.urgent} | tile: ${winDelegate.modelData.layout.tile_size ?? "none"} focusTs: ${winDelegate.modelData.focusTimestamp.secs ?? "never"}`

					MouseArea {
						anchors.fill: parent
						onClicked: winDelegate.modelData.activate()
					}
				}
			}
		}
	}
}
