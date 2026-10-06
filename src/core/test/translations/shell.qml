import QtQuick
import Quickshell

ShellRoot {
    id: root
    property string greeting: qsTr("Hello")
    property string initialGreeting
    property string initialItems
    property bool withFallback: Quickshell.env("QS_TEST_FALLBACK") === "1"
    property int count: 1
    property string files: qsTr("%n file(s)", "", count)
    // Deliberately absent from the Russian catalog.
    property string items: qsTr("%n item(s)", "", count)
    Component.onCompleted: {
        initialGreeting = qsTr("Hello");
        initialItems = items;
    }

    PersistentProperties {
        id: state
        reloadableId: "translationTest"
        property bool reloaded: false
    }

    Timer {
        interval: 1
        running: true
        onTriggered: {
            let ok = root.greeting === "Привет" && root.initialGreeting === "Привет";
            ok = ok && root.files === "1 файл";
            ok = ok && root.initialItems === (root.withFallback ? "1 item" : "1 item(s)");
            root.count = 2;
            ok = ok && root.files === "2 файла";
            root.count = 5;
            ok = ok && root.files === "5 файлов";
            ok = ok && root.items === (root.withFallback ? "5 items" : "5 item(s)");
            Qt.uiLanguage = "";
            ok = ok && root.greeting === "Hello";
            ok = ok && root.files === (root.withFallback ? "5 files" : "5 file(s)");
            root.count = 1;
            ok = ok && root.files === (root.withFallback ? "1 file" : "1 file(s)");
            Qt.uiLanguage = "de_DE";
            root.count = 2;
            ok = ok && root.greeting === "Hello";
            ok = ok && root.files === (root.withFallback ? "2 files" : "2 file(s)");
            Qt.uiLanguage = "ru_RU";
            ok = ok && root.greeting === "Привет" && root.files === "2 файла";
            if (ok && !state.reloaded) {
                state.reloaded = true;
                Quickshell.reload(false);
                return;
            }
            if (!ok) console.error("Translation checks failed");
            Qt.exit(ok ? 0 : 1);
        }
    }
}
