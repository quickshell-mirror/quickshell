import QtQuick
import Quickshell

ShellRoot {
    id: root
    property string greeting: qsTr("Hello")
    property string initialGreeting
    property int count: 1
    property string files: qsTr("%n file(s)", "", count)
    Component.onCompleted: initialGreeting = qsTr("Hello")

    Timer {
        interval: 1
        running: true
        onTriggered: {
            let ok = root.greeting === "Привет" && root.initialGreeting === "Привет";
            ok = ok && root.files === "1 файл";
            root.count = 2;
            ok = ok && root.files === "2 файла";
            root.count = 5;
            ok = ok && root.files === "5 файлов";
            Qt.uiLanguage = "";
            ok = ok && root.greeting === "Hello";
            Qt.uiLanguage = "ru_RU";
            ok = ok && root.greeting === "Привет" && root.files === "5 файлов";
            if (!ok) console.error("Translation checks failed");
            Qt.exit(ok ? 0 : 1);
        }
    }
}
