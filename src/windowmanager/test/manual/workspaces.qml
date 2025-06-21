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
				model: WindowManager.workspaceGroups

				WrapperRectangle {
					id: delegate
					required property WorkspaceGroup modelData
					color: "slategray"
					margin: 5

					ColumnLayout {
						Label { text: delegate.modelData.toString() }
						Label { text: `Screens: ${delegate.modelData.screens.map(s => s.name)}` }

						Repeater {
							model: ScriptModel {
								values: [...WindowManager.workspaces.values].filter(w => w.group == delegate.modelData)
							}

							WorkspaceDelegate {}
						}
					}
				}
			}

			Repeater {
				model: ScriptModel {
					values: WindowManager.workspaces.values.filter(w => w.group == null)
				}

				WorkspaceDelegate {}
			}
		}
	}

	component WorkspaceDelegate: WrapperRectangle {
		id: delegate
		required property Workspace modelData;
		color: modelData.active ? "green" : "gray"

		ColumnLayout {
			Label { text: delegate.modelData.toString() }
			Label { text: `Id: ${delegate.modelData.id} Name: ${delegate.modelData.name}` }

			RowLayout {
				Label { text: "Group:" }
				ComboBox {
					Layout.fillWidth: true
					implicitContentWidthPolicy: ComboBox.WidestText
					enabled: delegate.modelData.canSetGroup
					model: [...WindowManager.workspaceGroups.values].map(w => w.toString())
					currentIndex: WindowManager.workspaceGroups.values.indexOf(delegate.modelData.group)
					onActivated: i => delegate.modelData.setGroup(WindowManager.workspaceGroups.values[i])
				}
			}

			RowLayout {
				DisplayCheckBox {
					text: "Active"
					checked: delegate.modelData.active
				}

				DisplayCheckBox {
					text: "Urgent"
					checked: delegate.modelData.urgent
				}

				DisplayCheckBox {
					text: "Should Display"
					checked: delegate.modelData.shouldDisplay
				}
			}

			RowLayout {
				Button {
					text: "Activate"
					enabled: delegate.modelData.canActivate
					onClicked: delegate.modelData.activate()
				}

				Button {
					text: "Deactivate"
					enabled: delegate.modelData.canDeactivate
					onClicked: delegate.modelData.deactivate()
				}

				Button {
					text: "Remove"
					enabled: delegate.modelData.canRemove
					onClicked: delegate.modelData.remove()
				}
			}
		}
	}

	component DisplayCheckBox: CheckBox {
		enabled: false
		palette.disabled.text: parent.palette.active.text
		palette.disabled.windowText: parent.palette.active.windowText
	}
}
