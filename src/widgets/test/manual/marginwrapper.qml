import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Quickshell
import Quickshell.Widgets

FloatingWindow {
	color: contentItem.palette.window

	ColumnLayout {
		anchors.fill: parent

		Item {
			Layout.fillWidth: true
			Layout.fillHeight: true

			Rectangle {
				anchors.centerIn: parent
				width: stretchCb.checked ? wrapperWidthSlider.value : implicitWidth
				height: stretchCb.checked ? wrapperHeightSlider.value : implicitHeight
				border.color: "black"

				MarginWrapperManager {
					margin: marginSlider.value
					extraMargin: extraMarginSlider.value
					resizeChild: resizeCb.checked
					topMargin: separateMarginsCb.checked ? topMarginSlider.value : undefined
					bottomMargin: separateMarginsCb.checked ? bottomMarginSlider.value : undefined
					leftMargin: separateMarginsCb.checked ? leftMarginSlider.value : undefined
					rightMargin: separateMarginsCb.checked ? rightMarginSlider.value : undefined
					implicitWidth: parentImplicitSizeCb.checked ? parentImplicitWidthSlider.value : undefined
					implicitHeight: parentImplicitSizeCb.checked ? parentImplicitHeightSlider.value : undefined
				}

				Rectangle {
					color: "green"
					implicitWidth: implicitWidthSlider.value
					implicitHeight: implicitHeightSlider.value
				}
			}
		}

		RowLayout {
			Layout.fillWidth: true

			CheckBox {
				id: stretchCb
				text: "Stretch"
			}

			CheckBox {
				id: resizeCb
				text: "Resize Child"
			}

			CheckBox {
				id: separateMarginsCb
				text: "Individual Margins"
			}

			CheckBox {
				id: parentImplicitSizeCb
				text: "Parent Implicit Size"
			}
		}

		RowLayout {
			Layout.fillWidth: true

			Label { text: "Stretch Width" }
			Slider {
				id: wrapperWidthSlider
				Layout.fillWidth: true
				from: 0; to: 300; value: 200
			}

			Label { text: "Stretch Height" }
			Slider {
				id: wrapperHeightSlider
				Layout.fillWidth: true
				from: 0; to: 300; value: 200
			}
		}

		RowLayout {
			Layout.fillWidth: true

			Label { text: "Implicit Width" }
			Slider {
				id: implicitWidthSlider
				Layout.fillWidth: true
				from: 0; to: 200; value: 100
			}

			Label { text: "Implicit Height" }
			Slider {
				id: implicitHeightSlider
				Layout.fillWidth: true
				from: 0; to: 200; value: 100
			}
		}

		RowLayout {
			Layout.fillWidth: true

			Label { text: "Parent Implicit Width" }
			Slider {
				id: parentImplicitWidthSlider
				Layout.fillWidth: true
				from: 0; to: 300; value: 200
			}

			Label { text: "Parent Implicit Height" }
			Slider {
				id: parentImplicitHeightSlider
				Layout.fillWidth: true
				from: 0; to: 300; value: 200
			}
		}

		RowLayout {
			Layout.fillWidth: true

			Label { text: "Margin" }
			Slider {
				id: marginSlider
				Layout.fillWidth: true
				from: -100; to: 200; value: 50
			}

			Label { text: "Extra" }
			Slider {
				id: extraMarginSlider
				Layout.fillWidth: true
				from: -100; to: 200; value: 0
			}
		}

		RowLayout {
			Layout.fillWidth: true

			Label { text: "Top Margin" }
			Slider {
				id: topMarginSlider
				Layout.fillWidth: true
				from: -100; to: 200; value: 50
			}

			Label { text: "Bottom Margin" }
			Slider {
				id: bottomMarginSlider
				Layout.fillWidth: true
				from: -100; to: 200; value: 50
			}
		}

		RowLayout {
			Layout.fillWidth: true

			Label { text: "Left Margin" }
			Slider {
				id: leftMarginSlider
				Layout.fillWidth: true
				from: -100; to: 200; value: 50
			}

			Label { text: "Right Margin" }
			Slider {
				id: rightMarginSlider
				Layout.fillWidth: true
				from: -100; to: 200; value: 50
			}
		}
	}
}
