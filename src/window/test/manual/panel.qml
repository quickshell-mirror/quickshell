import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Quickshell

Scope {
	FloatingWindow {
		color: contentItem.palette.window
		minimumSize.width: layout.implicitWidth
		minimumSize.height: layout.implicitHeight

		ColumnLayout {
			id: layout

			RowLayout {
				CheckBox {
					id: visibleCb
					text: "Visible"
					checked: true
				}

				CheckBox {
					id: aboveCb
					text: "Above Windows"
					checked: true
				}
			}

			RowLayout {
				ColumnLayout {
					CheckBox {
						id: leftAnchorCb
						text: "Left"
					}

					SpinBox {
						id: leftMarginSb
						editable: true
						value: 0
						to: 1000
					}
				}

				ColumnLayout {
					CheckBox {
						id: rightAnchorCb
						text: "Right"
					}

					SpinBox {
						id: rightMarginSb
						editable: true
						value: 0
						to: 1000
					}
				}

				ColumnLayout {
					CheckBox {
						id: topAnchorCb
						text: "Top"
					}

					SpinBox {
						id: topMarginSb
						editable: true
						value: 0
						to: 1000
					}
				}

				ColumnLayout {
					CheckBox {
						id: bottomAnchorCb
						text: "Bottom"
					}

					SpinBox {
						id: bottomMarginSb
						editable: true
						value: 0
						to: 1000
					}
				}
			}

			RowLayout {
				ComboBox {
					id: exclusiveModeCb
					model: [ "Normal", "Ignore", "Auto" ]
					currentIndex: w.exclusionMode
				}

				SpinBox {
					id: exclusiveZoneSb
					editable: true
					value: 100
					to: 1000
				}
			}

			RowLayout {
				Label { text: "Width" }

				SpinBox {
					id: widthSb
					editable: true
					value: 100
					to: 1000
				}
			}

			RowLayout {
				Label { text: "Height" }

				SpinBox {
					id: heightSb
					editable: true
					value: 100
					to: 1000
				}
			}
		}
	}

	PanelWindow {
		id: w
		visible: visibleCb.checked
		aboveWindows: aboveCb.checked

		anchors {
			left: leftAnchorCb.checked
			right: rightAnchorCb.checked
			top: topAnchorCb.checked
			bottom: bottomAnchorCb.checked
		}

		margins {
			left: leftMarginSb.value
			right: rightMarginSb.value
			top: topMarginSb.value
			bottom: bottomMarginSb.value
		}

		exclusionMode: exclusiveModeCb.currentIndex
		exclusiveZone: exclusiveZoneSb.value

		implicitWidth: widthSb.value
		implicitHeight: heightSb.value
	}
}
