import QtQuick
import QtQuick.Layouts
import Quickshell
import Quickshell.Widgets
import Quickshell.Niri

FloatingWindow {
	Component.onCompleted: console.log("niri socket:", Niri.socketPath)

    Connections {
        target: Niri
        function onRawEvent(event: NiriEvent) {
            console.log("event:", event.name, event.data)
        }
    }
}
