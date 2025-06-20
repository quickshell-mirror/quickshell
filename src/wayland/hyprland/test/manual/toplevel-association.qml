import QtQuick
import QtQuick.Layouts
import Quickshell
import Quickshell.Hyprland
import Quickshell.Wayland

FloatingWindow {
	ColumnLayout {
		anchors.fill: parent

		Text { text: "Hyprland -> Wayland" }

		ListView {
			Layout.fillWidth: true
			Layout.fillHeight: true
			clip: true
			model: Hyprland.toplevels
			delegate: Text {
				required property HyprlandToplevel modelData
				text: `${modelData} -> ${modelData.wayland}`
			}
		}

		Text { text: "Wayland -> Hyprland" }

		ListView {
			Layout.fillWidth: true
			Layout.fillHeight: true
			clip: true
			model: ToplevelManager.toplevels
			delegate: Text {
				required property Toplevel modelData
				text: `${modelData} -> ${modelData.HyprlandToplevel.handle}`
			}
		}
	}
}
