import QtQuick
import QtQuick.Controls
import Quickshell
import Quickshell.Wayland

FloatingWindow {
    id: root
    color: "transparent"

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: enableCb.checked ? 0.5 : 1
    }

    CheckBox {
        id: enableCb
        anchors.centerIn: parent
        text: "Enable Blur"
        palette.windowText: "white"
    }

    BackgroundEffect {
        window: root
        enabled: enableCb.checked
        blurRegion: Region {
            item: root.contentItem
        }
    }
}
