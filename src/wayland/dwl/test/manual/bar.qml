import Quickshell
import Quickshell.Wayland
import Quickshell.DWL
import QtQuick
import QtQuick.Layouts

ShellRoot {
    Scope {
        Variants {
            model: Quickshell.screens
            PanelWindow {
                required property var modelData

                screen: modelData
                anchors.top: true
                anchors.left: true
                anchors.right: true
                implicitHeight: 30
                color: "#1e1e2e"

                property DwlIpcOutput dwlOutput: DwlIpc.outputs.length > 0 ? DwlIpc.outputForName(modelData.name) : null
                property string currentLayout: dwlOutput ? dwlOutput.kbLayout : ""

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 2

                    // Tag indicators
                    Repeater {
                        model: dwlOutput ? dwlOutput.tags : []
                        delegate: Rectangle {
                            required property DwlTag modelData
                            width: 22
                            height: 22
                            radius: 4
                            color: modelData.active ? "#89b4fa" : (modelData.clientCount > 0 ? "#313244" : "transparent")
                            border.color: modelData.urgent ? "#f38ba8" : "transparent"
                            border.width: 2

                            Text {
                                anchors.centerIn: parent
                                text: modelData.index + 1
                                color: modelData.active ? "#1e1e2e" : "#cdd6f4"
                                font.pixelSize: 12
                                font.bold: modelData.active
                            }

                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.LeftButton | Qt.RightButton
                                // Left click: switch to tag
                                // Right click: toggle tag on focused client
                                onClicked: mouse => {
                                    if (mouse.button === Qt.RightButton)
                                        dwlOutput.setClientTags(0xFFFFFFFF, 1 << modelData.index);
                                    else
                                        dwlOutput.setTags(1 << modelData.index);
                                }
                            }
                        }
                    }

                    // Layout symbol
                    Text {
                        text: dwlOutput ? dwlOutput.layoutSymbol : ""
                        color: "#a6e3a1"
                        font.pixelSize: 12
                        font.family: "monospace"
                        visible: dwlOutput !== null

                        MouseArea {
                            anchors.fill: parent

                            onClicked: {
                                if (dwlOutput) {
                                    const next = (dwlOutput.layoutIndex + 1) % DwlIpc.layouts.length;
                                    dwlOutput.setLayout(next);
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Text {
                        id: layoutText

                        text: {
                            if (!currentLayout)
                                return "XX";
                            if (currentLayout.includes('(') && currentLayout.includes(')')) {
                                const match = currentLayout.match(/\(([^)]+)\)/);
                                return match ? match[1] : currentLayout.substring(0, 2).toUpperCase();
                            }

                            const firstWord = currentLayout.split(' ')[0];
                            return firstWord.length <= 3 ? firstWord : firstWord.substring(0, 2).toUpperCase();
                        }

                        font.pixelSize: 12
                        font.bold: true
                        color: "#f38ba8"
                    }

                    // Focused window title
                    Text {
                        text: dwlOutput ? dwlOutput.title : ""
                        color: "#cdd6f4"
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        Layout.maximumWidth: 300
                    }

                    // Status indicators
                    Text {
                        visible: dwlOutput ? dwlOutput.fullscreen : false
                        text: "[F]"
                        color: "#fab387"
                        font.pixelSize: 12
                    }
                    Text {
                        visible: dwlOutput ? dwlOutput.floating : false
                        text: "[~]"
                        color: "#f9e2af"
                        font.pixelSize: 12
                    }
                }

                Text {
                    visible: !DwlIpc.available
                    anchors.centerIn: parent
                    text: "DWL not available"
                    color: "#f38ba8"
                    font.pixelSize: 12
                }
            }
        }
    }
}
