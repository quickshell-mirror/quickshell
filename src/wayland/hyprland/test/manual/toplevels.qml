import QtQuick
import QtQuick.Layouts
import Quickshell
import Quickshell.Hyprland

FloatingWindow {
	ColumnLayout {
		anchors.fill: parent

		Text { text: "Current toplevel:" }

		ToplevelFromHyprland {
			modelData: Hyprland.activeToplevel
		}

		Text { text: "\nAll toplevels:" }

		ListView {
			Layout.fillHeight: true
			Layout.fillWidth: true
			clip: true
			model: Hyprland.toplevels
			delegate: ToplevelFromHyprland {}
		}
	}

	component ToplevelFromHyprland: ColumnLayout {
		required property HyprlandToplevel modelData

		Text {
			text: `Window 0x${modelData.address}, title: ${modelData.title}, activated: ${modelData.activated}, workspace id: ${modelData.workspace.id}, monitor name: ${modelData.monitor.name}, urgent: ${modelData.urgent}`
		}
	}
}
