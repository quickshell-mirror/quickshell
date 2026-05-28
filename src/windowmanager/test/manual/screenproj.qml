import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import Quickshell
import Quickshell.Widgets
import Quickshell.WindowManager

FloatingWindow {
	ScrollView {
		anchors.fill: parent

		ColumnLayout {
			Repeater {
				model: Quickshell.screens

				WrapperRectangle {
					id: delegate
					required property ShellScreen modelData
					color: "slategray"
					margin: 5

					ColumnLayout {
						Label { text: `Screen: ${delegate.modelData.name}` }

						Repeater {
							model: ScriptModel {
								values: WindowManager.screenProjection(delegate.modelData).windowsets
							}

							WorkspaceDelegate {}
						}
					}
				}
			}

			Repeater {
				model: ScriptModel {
					values: WindowManager.windowsets.filter(w => w.projection == null)
				}

				WorkspaceDelegate {}
			}
		}
	}
}
