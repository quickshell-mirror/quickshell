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
				model: WindowManager.windowsetProjections

				WrapperRectangle {
					id: delegate
					required property WindowsetProjection modelData
					color: "slategray"
					margin: 5

					ColumnLayout {
						Label { text: delegate.modelData.toString() }
						Label { text: `Screens: ${delegate.modelData.screens.map(s => s.name)}` }

						Repeater {
							model: ScriptModel {
								values: delegate.modelData.windowsets
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
