import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Quickshell
import Quickshell.Io

Scope {
    id: root
    property real startTime: 0
    readonly property bool running: withControl.running || withoutControl.running

    function start() {
        startTime = Date.now() + 1500;
        withControl.running = true;
        withoutControl.running = true;
    }

    function stop() {
        withControl.running = false;
        withoutControl.running = false;
    }

    component TestProcess: Process {
        required property string config
        command: ["/proc/" + Quickshell.processId + "/exe", "--path", Quickshell.shellPath(config)]
        environment: ({
                QS_RENDERCONTROL_START: String(root.startTime)
            })
        onExited: root.stop()
        stdout: SplitParser {
            onRead: data => console.log(data)
        }
        stderr: SplitParser {
            onRead: data => console.warn(data)
        }
    }

    TestProcess {
        id: withControl
        config: "rendercontrol-enabled.qml"
    }

    TestProcess {
        id: withoutControl
        config: "rendercontrol-qt.qml"
    }

    PanelWindow {
        anchors.top: true
        anchors.left: true
        anchors.right: true
        implicitHeight: 48
        exclusiveZone: 48
        color: "#202020"

        RowLayout {
            anchors.centerIn: parent

            Button {
                text: root.running ? "Stop comparison" : "Start comparison"
                onClicked: root.running ? root.stop() : root.start()
            }

            Label {
                text: "RenderControl left | Qt render loop right | 4 seconds per test, repeating"
                color: "white"
            }
        }
    }
}
