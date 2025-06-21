import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import Quickshell
import Quickshell.Widgets
import Quickshell.WindowManager

FloatingWindow {
	ColumnLayout {
		Repeater {
			model: WindowManager.workspaces

			WrapperRectangle {
				id: delegate
				required property Workspace modelData;
				color: modelData.active ? "limegreen" : "gray"

				ColumnLayout {
					Label { text: delegate.modelData.toString() }
					Label { text: `Id: ${delegate.modelData.id} Name: ${delegate.modelData.name}` }

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
		}
	}

	component DisplayCheckBox: CheckBox {
		enabled: false
		palette.disabled.text: parent.palette.active.text
		palette.disabled.windowText: parent.palette.active.windowText
	}
}
