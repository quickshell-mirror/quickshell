import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Quickshell
import Quickshell.Wayland

FloatingWindow {
  id: root
  color: "transparent"
	contentItem.palette.windowText: "white"

	ColumnLayout {
		anchors.centerIn: parent

		CheckBox {
			id: enableBox
			checked: true
			text: "Enable Blur"
		}

		Button {
			text: "Hide->Show"
			onClicked: {
				root.visible = false
				showTimer.start()
			}
		}

		Timer {
			id: showTimer
			interval: 200
			onTriggered: root.visible = true
		}

		Slider {
			id: radiusSlider
			from: 0
			to: 1000
			value: 100
		}

		component EdgeSlider: Slider {
			from: -1
			to: 1000
			value: -1
		}

		EdgeSlider { id: topLeftSlider }
		EdgeSlider { id: topRightSlider }
		EdgeSlider { id: bottomLeftSlider }
		EdgeSlider { id: bottomRightSlider }
	}

	BackgroundEffect.blurRegion: Region {
		item: enableBox.checked ? root.contentItem : null
		radius: radiusSlider.value == -1 ? undefined : radiusSlider.value
		topLeftRadius: topLeftSlider.value == -1 ? undefined : topLeftSlider.value
		topRightRadius: topRightSlider.value == -1 ? undefined : topRightSlider.value
		bottomLeftRadius: bottomLeftSlider.value == -1 ? undefined : bottomLeftSlider.value
		bottomRightRadius: bottomRightSlider.value == -1 ? undefined : bottomRightSlider.value
	}
}
