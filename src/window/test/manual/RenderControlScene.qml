import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Quickshell

Scope {
    id: root
    required property bool renderControl
    readonly property string rendererName: renderControl ? "RenderControl" : "Qt render loop"
    readonly property real startTime: Number(Quickshell.env("QS_RENDERCONTROL_START")) || Date.now()
    readonly property int testDuration: 4000
    property real elapsed: -1
    readonly property int testIndex: elapsed < 0 ? -1 : Math.floor(elapsed / testDuration) % tests.length
    readonly property string test: testIndex < 0 ? "Waiting" : tests[testIndex]
    readonly property bool decorated: test === "Decorated window"
    property bool effectsLoaded: false
    property string effectsStatus: ""
    property bool viewUnavailable: false
    property string status: ""
    readonly property var tests: ["Idle", "Move", "Create and destroy", "Move child, then parent", "Move row, change text", "Resize clip", "Resize under scale", "XAnimator", "OpacityAnimator", "NumberAnimation and RotationAnimator", "Recolor antialiased", "Transform antialiased", "Hide antialiased", "Hidden changes", "Move text", "New glyphs", "Recolor 40 rects", "Perspective", "Custom vertex shader", "Recolor effect sources", "Hide effect chain middle", "Window color", "Swap pages", "Load effects", "Animate effects", "View3D Offscreen", "View3D Underlay", "View3D Overlay", "View3D Inline", "Popup", "Resize panel", "Click-through panel", "Hide and show", "Disable updates", "Decorated window"]

    onTestChanged: {
        if (testIndex < 0)
            return;
        status = "";
        if (test === "Load effects" && !effectsLoaded) {
            effectsLoaded = true;
            try {
                effectsStatus = scene.loadEffects();
            } catch (error) {
                effectsStatus = "QtQuick.Effects is not available";
            }
        }
        if (test === "Load effects" || test === "Animate effects")
            status = effectsStatus;
        if (test.startsWith("View3D ")) {
            status = scene.setView(test.slice(7));
        } else if (scene.view) {
            scene.view.mode = "None";
        }
        if (test !== "Swap pages")
            stack.replace(tallPage, StackView.Immediate);
    }

    Timer {
        interval: 50
        running: true
        repeat: true
        onTriggered: root.elapsed = Date.now() - root.startTime
    }

    QtObject {
        id: tick
        readonly property int count: Math.max(0, Math.floor(root.elapsed / 500))
        readonly property bool on: count % 2 !== 0
        onCountChanged: {
            if (root.test === "Swap pages")
                stack.replace(on ? shortPage : tallPage, StackView.Immediate);
        }
    }

    PanelWindow {
        id: window
        anchors.top: true
        anchors.bottom: true
        anchors.left: root.renderControl
        anchors.right: !root.renderControl
        margins.bottom: 90
        implicitWidth: screen ? Math.floor(screen.width / 2) : 800
        exclusionMode: ExclusionMode.Normal
        visible: !root.decorated && (root.test !== "Hide and show" || Math.floor(root.elapsed / 1000) % 2 === 0)
        updatesEnabled: root.test !== "Disable updates" || tick.on
        color: root.test === "Window color" && tick.on ? "#606060" : "#202020"
    }

    FloatingWindow {
        id: decoratedWindow
        title: root.rendererName
        visible: root.decorated
        implicitWidth: 800
        implicitHeight: 660
        color: "#202020"
    }

    Item {
        parent: root.decorated ? decoratedWindow.contentItem : window.contentItem
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 4

            Label {
                text: root.rendererName + " | " + (root.testIndex + 1) + "/" + root.tests.length + " | " + root.test
                color: "white"
            }

            Label {
                visible: root.status !== ""
                text: root.status
                color: "white"
            }

            Label {
                visible: root.test === "Hidden changes"
                text: "Hidden swaps: " + hiddenChange.swaps + "/" + hiddenChange.changes
                color: "white"
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                implicitWidth: 800
                implicitHeight: 600
                Rectangle {
                    id: scene
                    width: 800
                    height: 600
                    scale: Math.min(parent.width / width, parent.height / height)
                    transformOrigin: Item.TopLeft
                    color: "black"

                    property Item view: null

                    // Test shader detection when QtQuick.Effects loads after the tracker.
                    function loadEffects(): string {
                        const late = Qt.createQmlObject(`import QtQuick; import QtQuick.Effects; MultiEffect { anchors.fill: parent; brightness: root.test === "Animate effects" && tick.on ? 0.5 : 0 }`, lateCell);
                        late.source = lateSource;

                        Qt.createQmlObject(`import QtQuick; import QtQuick.Effects; Rectangle {
    						anchors.fill: parent
    						color: (root.test === "Animate effects") && tick.on ? "gray" : "white"
    						layer.enabled: true
    						layer.effect: MultiEffect { shadowEnabled: true; shadowColor: "orange"; shadowHorizontalOffset: 20; shadowBlur: 0.5 }
    					}`, shadowCell);

                        // Test damage carried into the next frame when the mask layer updates.
                        Qt.createQmlObject(`import QtQuick; import QtQuick.Effects; Item {
    						anchors.fill: parent
    						layer.enabled: true
    						layer.effect: MultiEffect { maskEnabled: true; maskSource: maskSource }

    						Rectangle { anchors.fill: parent; color: (root.test === "Animate effects") && tick.on ? "yellow" : "white" }

    						Rectangle {
    							id: maskSource
    							anchors.fill: parent
    							visible: false
    							layer.enabled: true
    							gradient: Gradient {
    								orientation: Gradient.Horizontal
    								GradientStop { position: 0; color: "transparent" }
    								GradientStop { position: 1; color: "white" }
    							}
    						}
    					}`, maskCell);

                        try {
                            Qt.createQmlObject(`import QtQuick; import QtQuick.Effects; RectangularShadow {
    							anchors.fill: parent
    							blur: 10
    							offset.x: 6
    							offset.y: 6
    							color: (root.test === "Animate effects") && tick.on ? "orange" : "white"
    						}`, rectShadowCell);
                        } catch (error) {
                            return "Loaded, no RectangularShadow (Qt 6.9)";
                        }

                        return "Loaded";
                    }

                    function setView(mode: string): string {
                        if (root.viewUnavailable)
                            return "QtQuick3D is not available";
                        if (!scene.view) {
                            try {
                                scene.view = Qt.createQmlObject(`import QtQuick; import QtQuick3D; View3D {
    								property string mode
    								anchors.fill: parent
    								visible: mode != "None"
    								renderMode: mode == "Underlay" ? View3D.Underlay : mode == "Overlay" ? View3D.Overlay : mode == "Inline" ? View3D.Inline : View3D.Offscreen
    								environment: SceneEnvironment { backgroundMode: SceneEnvironment.Color; clearColor: "#303030" }
    								PerspectiveCamera { z: 300 }
    								DirectionalLight {}
    								Model {
    									source: "#Cube"
    									materials: DefaultMaterial { diffuseColor: "red" }
    									NumberAnimation on eulerRotation.y { from: 0; to: 360; duration: 2000; loops: Animation.Infinite; running: root.test.startsWith("View3D ") }
    								}
    							}`, viewCell);
                            } catch (error) {
                                root.viewUnavailable = true;
                                return "QtQuick3D is not available";
                            }
                        }

                        scene.view.mode = mode;
                        return "";
                    }

                    Rectangle {
                        x: (root.test === "Move" || root.decorated) && tick.on ? 760 : 430
                        y: 10
                        width: 20
                        height: 20
                        color: "red"
                    }

                    Loader {
                        x: 460
                        y: 10
                        active: !((root.test === "Create and destroy") && tick.on)
                        sourceComponent: Rectangle {
                            width: 20
                            height: 20
                            color: "orange"
                        }
                    }

                    Item {
                        x: 430
                        y: 50
                        width: (root.test === "Resize clip") && tick.on ? 150 : 50
                        height: 30
                        clip: true

                        Rectangle {
                            width: 300
                            height: 30
                            color: "green"
                        }
                    }

                    Item {
                        x: (root.test === "Move child, then parent") && tick.count % 4 >= 2 ? 630 : 600
                        y: 100
                        width: 40
                        height: 40

                        Rectangle {
                            x: (root.test === "Move child, then parent") && tick.count % 2 ? 150 : 0
                            y: (root.test === "Move child, then parent") && tick.count % 2 ? 40 : 0
                            width: 20
                            height: 20
                            color: "yellow"
                        }
                    }

                    Item {
                        x: 480
                        y: 150
                        width: (root.test === "Resize under scale") && tick.on ? 80 : 40
                        height: 40
                        scale: 2

                        Rectangle {
                            width: 20
                            height: 20
                            color: "cyan"
                        }
                    }

                    Rectangle {
                        id: xMoved
                        x: 430
                        y: 200
                        width: 20
                        height: 20
                        color: "blue"
                    }

                    XAnimator {
                        target: xMoved
                        running: (root.test === "XAnimator")
                        alwaysRunToEnd: true
                        from: 760
                        to: 430
                        duration: 1000
                        loops: Animation.Infinite
                    }

                    Rectangle {
                        id: faded
                        x: 430
                        y: 240
                        width: 20
                        height: 20
                        color: "magenta"
                    }

                    OpacityAnimator {
                        target: faded
                        running: (root.test === "OpacityAnimator")
                        alwaysRunToEnd: true
                        from: 0.2
                        to: 1
                        duration: 1000
                        loops: Animation.Infinite
                    }

                    Item {
                        x: (root.test === "Move row, change text") && tick.on ? 440 : 430
                        y: 270

                        Rectangle {
                            width: (root.test === "Move row, change text") && tick.on ? 160 : 60
                            height: 20
                            color: "teal"
                        }

                        Text {
                            x: 4
                            y: 2
                            text: (root.test === "Move row, change text") && tick.on ? "moved and longer" : "row"
                            font.pixelSize: 14
                            color: "white"
                        }
                    }

                    Item {
                        x: 10
                        y: 10
                        width: 350
                        height: 150
                        visible: !((root.test === "Hide antialiased") && tick.on)
                        rotation: (root.test === "Transform antialiased") && tick.on ? 17 : 0
                        scale: (root.test === "Transform antialiased") && tick.count % 4 >= 2 ? 0.6 : 1

                        Repeater {
                            model: 8

                            Item {
                                required property int index
                                x: 10.5 + index % 4 * 85
                                y: 10.5 + Math.floor(index / 4) * 75
                                transform: Matrix4x4 {
                                    matrix: Qt.matrix4x4(1, index == 6 ? 0.8 : 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
                                }

                                Rectangle {
                                    width: index == 0 ? 21 : 60
                                    height: index == 0 ? 21 : index == 7 ? 0.3 : 36
                                    radius: index % 2 ? 12 : 0
                                    rotation: index == 2 ? 33 : index == 4 ? 75 : 0
                                    scale: index == 5 ? -0.7 : 1
                                    border.width: index == 3 ? 6 : 0
                                    border.color: "orange"
                                    antialiasing: true
                                    color: (root.test === "Recolor antialiased") && tick.on ? "green" : "red"
                                }
                            }
                        }
                    }

                    Item {
                        x: (root.test === "Transform antialiased") && tick.on ? 21 : 10.5
                        y: 240.5

                        Repeater {
                            model: 3

                            Image {
                                required property int index
                                x: index * 80
                                width: 50
                                height: 34
                                rotation: index * 27
                                antialiasing: true
                                source: "data:image/svg+xml," + encodeURIComponent("<svg xmlns='http://www.w3.org/2000/svg' width='32' height='32'><rect width='32' height='32' fill='white'/><circle cx='16' cy='16' r='10' fill='orange'/></svg>")
                            }
                        }
                    }

                    Rectangle {
                        id: lateSource
                        width: 100
                        height: 100
                        color: (root.test === "Animate effects") && tick.on ? "blue" : "green"
                        visible: false
                    }

                    Item {
                        id: lateCell
                        x: 10
                        y: 310
                        width: 100
                        height: 100
                    }

                    Rectangle {
                        id: customVertexSource
                        width: 40
                        height: 40
                        color: "red"
                        visible: false
                    }

                    // Test Qt's default shader supplied explicitly as a custom vertexShader.
                    ShaderEffect {
                        x: 120
                        y: 310
                        width: 40
                        height: 40
                        visible: root.test === "Custom vertex shader" && tick.on
                        property variant source: ShaderEffectSource {
                            sourceItem: customVertexSource
                            hideSource: true
                        }
                        vertexShader: "qrc:/qt-project.org/scenegraph/shaders_ng/shadereffect.vert.qsb"
                    }

                    Rectangle {
                        id: sharedSource
                        x: 180
                        y: 310
                        width: 30
                        height: 30
                        color: (root.test === "Recolor effect sources") && tick.on ? "white" : "blue"
                    }

                    ShaderEffectSource {
                        x: 180
                        y: 360
                        width: 30
                        height: 30
                        sourceItem: sharedSource
                    }

                    ShaderEffectSource {
                        x: 220
                        y: 360
                        width: 30
                        height: 30
                        sourceItem: sharedSource
                    }

                    Item {
                        id: hiddenSource
                        x: 270
                        y: 310
                        width: 30
                        height: 30

                        Rectangle {
                            anchors.fill: parent
                            color: (root.test === "Recolor effect sources") && tick.on ? "purple" : "green"
                        }
                    }

                    ShaderEffect {
                        x: 270
                        y: 360
                        width: 30
                        height: 30
                        property variant source: ShaderEffectSource {
                            sourceItem: hiddenSource
                            hideSource: true
                        }
                    }

                    Item {
                        id: shadowCell
                        x: 360
                        y: 330
                        width: 40
                        height: 40
                    }

                    Item {
                        id: rectShadowCell
                        x: 500
                        y: 330
                        width: 60
                        height: 40
                    }

                    Component {
                        id: tallPage
                        Item {
                            Rectangle {
                                width: 70
                                height: 100
                                color: "yellow"
                            }
                        }
                    }

                    Component {
                        id: shortPage
                        Item {
                            Rectangle {
                                width: 70
                                height: 30
                                color: "blue"
                            }
                        }
                    }

                    Loader {
                        x: 620
                        y: 300
                        width: 70
                        height: 110
                        sourceComponent: (root.test === "Swap pages") && tick.on ? shortPage : tallPage
                    }

                    StackView {
                        id: stack
                        x: 710
                        y: 300
                        width: 70
                        height: 110
                        initialItem: tallPage
                    }

                    Item {
                        x: (root.test === "Move text") && tick.on ? 10.75 : 10.25
                        y: (root.test === "Move text") && tick.count % 4 >= 2 ? 441 : 440.5

                        Repeater {
                            model: 8

                            Text {
                                required property int index
                                x: index < 4 ? 0 : 150
                                y: index % 4 * 22
                                text: "gjpq Áç 0123"
                                font.pixelSize: 14
                                color: "white"
                                style: [Text.Normal, Text.Outline, Text.Raised, Text.Sunken][index % 4]
                                styleColor: "orange"
                                renderType: index < 4 ? Text.QtRendering : Text.NativeRendering
                            }
                        }
                    }

                    Text {
                        x: 10
                        y: 540
                        text: (root.test === "New glyphs") ? String.fromCodePoint(0x41 + tick.count % 58, 0x100 + tick.count % 200) : ""
                        font.pixelSize: 14
                        color: "white"
                    }

                    Grid {
                        x: 440
                        y: 440
                        columns: 10
                        spacing: 8

                        Repeater {
                            model: 40
                            Rectangle {
                                width: 8
                                height: 8
                                color: (root.test === "Recolor 40 rects") && tick.on ? "green" : "red"
                            }
                        }
                    }

                    Rectangle {
                        x: 620
                        y: 440
                        width: 60
                        height: 40
                        visible: root.test === "Perspective"
                        color: (root.test === "Perspective") && tick.on ? "yellow" : "blue"
                        transform: Rotation {
                            origin.x: 30
                            origin.y: 20
                            axis {
                                x: 1
                                y: 0
                                z: 0
                            }
                            angle: 40
                        }
                    }

                    Rectangle {
                        id: chainSource
                        width: 30
                        height: 30
                        color: (root.test === "Recolor effect sources") && tick.on ? "green" : "red"
                        visible: false
                    }

                    ShaderEffectSource {
                        id: chainStart
                        sourceItem: chainSource
                        hideSource: true
                        visible: false
                        width: 30
                        height: 30
                    }

                    Item {
                        id: chainItem
                        x: 440
                        y: 520
                        width: 30
                        height: 30

                        ShaderEffect {
                            width: 30
                            height: 30
                            property variant source: chainStart
                        }
                    }

                    ShaderEffectSource {
                        id: chainMiddle
                        x: 480
                        y: 520
                        width: 30
                        height: 30
                        visible: !((root.test === "Hide effect chain middle") && tick.on)
                        sourceItem: chainItem
                        hideSource: true
                    }

                    ShaderEffect {
                        x: 520
                        y: 520
                        width: 30
                        height: 30
                        property variant source: chainMiddle
                    }

                    ShaderEffect {
                        x: 560
                        y: 520
                        width: 30
                        height: 30
                        property variant source: chainMiddle
                    }

                    Item {
                        id: maskCell
                        x: 60
                        y: 560
                        width: 40
                        height: 30
                    }

                    Rectangle {
                        x: 490
                        y: 232
                        width: 30
                        height: 30
                        color: "orange"

                        NumberAnimation on rotation {
                            from: 0
                            to: 360
                            duration: 1500
                            loops: Animation.Infinite
                            running: (root.test === "NumberAnimation and RotationAnimator")
                        }
                    }

                    Rectangle {
                        x: 540
                        y: 232
                        width: 30
                        height: 30
                        color: "steelblue"

                        RotationAnimator on rotation {
                            from: 0
                            to: 360
                            duration: 1500
                            loops: Animation.Infinite
                            running: (root.test === "NumberAnimation and RotationAnimator")
                        }
                    }

                    Item {
                        id: viewCell
                        x: 620
                        y: 500
                        width: 150
                        height: 90
                    }
                }
            }
        }
    }

    PopupWindow {
        anchor.window: window
        anchor.rect.x: Math.max(0, window.width - width)
        anchor.rect.y: 80
        implicitWidth: 180
        implicitHeight: 80
        visible: root.test === "Popup" && tick.on
        color: "#804020"

        Label {
            anchors.centerIn: parent
            text: root.rendererName + " popup"
            color: "white"
        }
    }

    PanelWindow {
        id: panel
        screen: window.screen
        anchors.bottom: true
        anchors.left: root.renderControl
        anchors.right: !root.renderControl
        implicitWidth: screen ? Math.floor(screen.width / 2) : 800
        implicitHeight: root.test === "Resize panel" && tick.on ? 80 : 40
        exclusionMode: ExclusionMode.Normal
        color: "#304030"
        mask: root.test === "Click-through panel" && tick.on ? emptyRegion : null

        Region {
            id: emptyRegion
        }

        Label {
            anchors.verticalCenter: parent.verticalCenter
            x: 10
            color: "white"
            text: root.rendererName + " panel, " + panel.height + " px"
        }

        Item {
            id: hiddenChange
            anchors.right: parent.right
            width: 20
            height: 20
            opacity: 0
            visible: root.test === "Hidden changes" && tick.on
            property int changes: 0
            property int swaps: 0
            property bool pending: false

            onVisibleChanged: {
                changes++;
                pending = true;
            }

            Rectangle {
                anchors.fill: parent
                color: "white"
            }

            Connections {
                target: hiddenChange.Window.window
                function onFrameSwapped() {
                    if (!hiddenChange.pending)
                        return;
                    hiddenChange.pending = false;
                    hiddenChange.swaps++;
                }
            }
        }
    }
}
