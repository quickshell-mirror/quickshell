pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Quickshell
import Quickshell.Widgets

PanelWindow {
	id: root
	required property ReloadPopupInfo reloadInfo
	readonly property string instanceId: root.reloadInfo.instanceId
	readonly property bool failed: root.reloadInfo.failed
	readonly property string errorString: root.reloadInfo.errorString

	anchors { left: true; top: true }
	margins { left: 25; top: 25 }

	implicitWidth: wrapper.implicitWidth
	implicitHeight: wrapper.implicitHeight

	color: "transparent"

	focusable: failText.focus

	// Composite before changing opacity
	SequentialAnimation on contentItem.opacity {
		id: fadeOutAnim
		NumberAnimation {
			// avoids 0 which closes the popup
			from: 0.0001; to: 1
			duration: 250
			easing.type: Easing.OutQuad
		}
		PauseAnimation { duration: root.failed ? 2000 : 500 }
		NumberAnimation {
			to: 0
			duration: root.failed ? 3000 : 800
			easing.type: Easing.InQuad
		}
	}

	Behavior on contentItem.opacity {
		enabled: !fadeOutAnim.running

		NumberAnimation {
			duration: 250
			easing.type: Easing.OutQuad
		}
	}

	contentItem.onOpacityChanged: {
		if (contentItem.opacity == 0) root.reloadInfo.closed()
	}

	component PopupText: Text {
		color: palette.active.text
	}

	component TopButton: WrapperMouseArea {
		id: buttonMouse
		required property string icon
		required property string fallbackText
		property bool red: false

		hoverEnabled: true

		WrapperRectangle {
			radius: 5

			color: {
				if (buttonMouse.red) {
					const baseColor = "#c04040";
					if (buttonMouse.pressed) return Qt.tint(palette.active.button, Qt.alpha(baseColor, 0.8));
					if (buttonMouse.containsMouse) return baseColor;
				} else {
					if (buttonMouse.pressed) return Qt.tint(palette.active.button, Qt.alpha(palette.active.accent, 0.3));
					if (buttonMouse.containsMouse) return Qt.tint(palette.active.button, Qt.alpha(palette.active.accent, 0.5));
				}

				return palette.active.button;
			}

			border.color: {
				if (buttonMouse.red) {
					const baseColor = "#c04040";
					if (buttonMouse.pressed) return Qt.tint(palette.active.light, Qt.alpha(baseColor, 0.8));
					if (buttonMouse.containsMouse) return baseColor;
				} else {
					if (buttonMouse.pressed) return Qt.tint(palette.active.light, Qt.alpha(palette.active.accent, 0.7));
					if (buttonMouse.containsMouse) return palette.active.accent;
				}

				return palette.active.light;
			}

			Behavior on color { ColorAnimation { duration: 100 } }
			Behavior on border.color { ColorAnimation { duration: 100 } }

			IconImage {
				id: image
				source: Quickshell.iconPath(buttonMouse.icon, true)
				implicitSize: 22
				visible: source != ""
			}

			Text {
				id: fallback
				text: buttonMouse.fallbackText
				color: buttonMouse.red ? "white" : palette.active.buttonText
			}

			child: image.visible ? image : fallback
		}
	}

	WrapperRectangle {
		id: wrapper
		anchors.fill: parent
		color: palette.active.window
		border.color: root.failed ? "#b53030" : palette.active.accent
		border.width: 1

		radius: 10
		margin: 10

		HoverHandler {
			onHoveredChanged: {
				if (hovered && fadeOutAnim.running) {
					fadeOutAnim.stop();
					root.contentItem.opacity = 1;
				}
			}
		}

		ColumnLayout {
			RowLayout {
				PopupText {
					font.pixelSize: 20
					fontSizeMode: Text.VerticalFit
					text: `Quickshell: ${root.failed ? "Config reload failed" : "Config reloaded"}`
				}

				Item { Layout.fillWidth: true }

				TopButton {
					id: copyButton
					visible: root.failed
					icon: "edit-copy"
					fallbackText: "Copy"
					onClicked: {
						Quickshell.clipboardText = root.errorString;
						copyTooltip.showAction();
					}
				}

				Tooltip {
					id: copyTooltip
					anchorItem: copyButton
					show: copyButton.containsMouse
					text: "Copy error message"
					actionText: "Copied to clipboard"
				}

				TopButton {
					icon: "window-close"
					fallbackText: "Close"
					red: true
					onClicked: {
						fadeOutAnim.stop()
						root.contentItem.opacity = 0
					}
				}
			}

			WrapperRectangle {
				visible: root.failed
				color: palette.active.base
				margin: 10
				radius: 5

				TextEdit {
					id: failText
					text: root.errorString
					color: palette.active.text
					selectionColor: palette.active.highlight
					selectedTextColor: palette.active.highlightedText
					readOnly: true
					font.family: "monospace"
				}
			}

			RowLayout {
				PopupText { text: "Run" }

				WrapperMouseArea {
					id: logButton

					Layout.topMargin: -logWrapper.margin
					Layout.bottomMargin: -logWrapper.margin

					hoverEnabled: true

					onPressed: {
						Quickshell.clipboardText = logText.text;
						logCopyTooltip.showAction();
					}

					WrapperRectangle {
						id: logWrapper
						margin: 2
						radius: 5

						color: {
							if (logButton.pressed) return Qt.tint(palette.active.base, Qt.alpha(palette.active.accent, 0.1));
							if (logButton.containsMouse) return Qt.tint(palette.active.base, Qt.alpha(palette.active.accent, 0.2));
							return palette.active.base;
						}

						border.color: {
							if (logButton.pressed) return Qt.tint(palette.active.button, Qt.alpha(palette.active.accent, 0.3));
							if (logButton.containsMouse) return Qt.tint(palette.active.button, Qt.alpha(palette.active.accent, 0.5));
							return palette.active.button;
						}

						Behavior on color { ColorAnimation { duration: 100 } }
						Behavior on border.color { ColorAnimation { duration: 100 } }

						RowLayout {
							PopupText {
								id: logText
								text: `qs log -i ${root.instanceId}`
							}

							IconImage {
								Layout.fillHeight: true
								implicitWidth: height
								source: Quickshell.iconPath("edit-copy", true)
								visible: source != ""
							}
						}
					}

					Tooltip {
						id: logCopyTooltip
						anchorItem: logWrapper
						show: logButton.containsMouse
						text: "Copy command"
						actionText: "Copied to clipboard"
					}
				}

				PopupText { text: "to view the log." }
			}
		}
	}
}
