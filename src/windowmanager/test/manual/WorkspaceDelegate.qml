import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import Quickshell
import Quickshell.Widgets
import Quickshell.WindowManager

WrapperRectangle {
	id: delegate
	required property Windowset modelData;
	color: modelData.active ? "green" : "gray"

	ColumnLayout {
		Label { text: delegate.modelData.toString() }
		Label { text: `Id: ${delegate.modelData.id} Name: ${delegate.modelData.name}` }
		Label { text: `Coordinates: ${delegate.modelData.coordinates.toString()}`}

		RowLayout {
			Label { text: "Group:" }
			ComboBox {
				Layout.fillWidth: true
				implicitContentWidthPolicy: ComboBox.WidestText
				enabled: delegate.modelData.canSetProjection
				model: [...WindowManager.windowsetProjections].map(w => w.toString())
				currentIndex: WindowManager.windowsetProjections.indexOf(delegate.modelData.projection)
				onActivated: i => delegate.modelData.setProjection(WindowManager.windowsetProjections[i])
			}
		}

		RowLayout {
			Label { text: "Screen:" }
			ComboBox {
				Layout.fillWidth: true
				implicitContentWidthPolicy: ComboBox.WidestText
				enabled: delegate.modelData.canSetProjection
				model: [...Quickshell.screens].map(w => w.name)
				currentIndex: Quickshell.screens.indexOf(delegate.modelData.projection.screens[0])
				onActivated: i => delegate.modelData.setProjection(WindowManager.screenProjection(Quickshell.screens[i]))
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

	component DisplayCheckBox: CheckBox {
		enabled: false
		palette.disabled.text: parent.palette.active.text
		palette.disabled.windowText: parent.palette.active.windowText
	}
}
