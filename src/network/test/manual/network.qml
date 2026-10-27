import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Quickshell
import Quickshell.Widgets
import Quickshell.Networking

Scope {
    Component {
        id: editorComponent
        FloatingWindow {
            id: editorWindow
            required property var nmSettings
            color: contentItem.palette.window

            Component.onCompleted: editorArea.text = JSON.stringify(nmSettings.read(), null, 2)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10

                Label {
                    text: "Editing " + nmSettings?.id + " (" + nmSettings?.uuid + ")"
                    font.bold: true
                    font.pointSize: 12
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    TextArea {
                        id: editorArea
                        wrapMode: TextEdit.Wrap
                        selectByMouse: true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        id: statusLabel
                        Layout.fillWidth: true
                        color: palette.placeholderText
                    }
                    Button {
                        text: "Reload"
                        onClicked: {
                            editorArea.text = JSON.stringify(editorWindow.nmSettings.read(), null, 2);
                            statusLabel.text = "Reloaded";
                        }
                    }
                    Button {
                        text: "Save"
                        onClicked: {
                            try {
                                const parsed = JSON.parse(editorArea.text);
                                nmSettings.write(parsed);
                                statusLabel.text = "Saved";
                            } catch (e) {
                                statusLabel.text = "Parse error: " + e.message;
                            }
                        }
                    }
                    Button {
                        text: "Close"
                        onClicked: {
                            editorArea.focus = false;
                            editorWindow.destroy();
                        }
                    }
                }
            }
        }
    }

    Component {
        id: deviceDelegate
        WrapperRectangle {
            width: parent.width
            color: "transparent"
            border.color: palette.button
            border.width: 1
            margin: 5

            ColumnLayout {
                RowLayout {
                    Label {
                        text: modelData.name
                        font.bold: true
                    }
                    Label {
                        text: modelData.address
                    }
                    Label {
                        text: `(Type: ${DeviceType.toString(modelData.type)})`
                    }
                    CheckBox {
                        text: `Managed`
                        checked: modelData.nmManaged
                        onClicked: modelData.nmManaged = !modelData.nmManaged
                    }
                }
                RowLayout {
                    Label {
                        text: ConnectionState.toString(modelData.state)
                        color: modelData.connected ? palette.link : palette.placeholderText
                    }
                    Button {
                        visible: modelData.state == ConnectionState.Connected
                        text: "Disconnect"
                        onClicked: modelData.disconnect()
                    }
                    CheckBox {
                        text: "Autoconnect"
                        checked: modelData.autoconnect
                        onClicked: modelData.autoconnect = !modelData.autoconnect
                    }
                    RowLayout {
                        visible: modelData.type === DeviceType.Wired
                        CheckBox {
                            text: "Link connected"
                            checked: modelData.hasLink
                            enabled: false
                        }
                        Label {
                            text: `Link max speed: ${modelData.linkSpeed} Mbps`
                        }
                    }
                    RowLayout {
                        visible: modelData.type === DeviceType.Wifi
                        Label {
                            text: `Mode: ${WifiDeviceMode.toString(modelData.mode)}`
                            visible: modelData.type == DeviceType.Wifi
                        }
                        CheckBox {
                            text: "Scanner"
                            checked: modelData.scannerEnabled
                            onClicked: modelData.scannerEnabled = !modelData.scannerEnabled
                            visible: modelData.type === DeviceType.Wifi
                        }
                    }
                }

                Repeater {
                    model: ScriptModel {
                        values: [...modelData.networks.values].sort((a, b) => {
                            if (a.connected !== b.connected) {
                                return b.connected - a.connected;
                            }
                            if (modelData.type === DeviceType.Wifi) {
                                return b.signalStrength - a.signalStrength;
                            }
                        })
                    }

                    WrapperRectangle {
                        id: ethernetNetwork
                        property var chosenSettings: {
                            const settings = modelData.nmSettings;
                            if (!settings || settings.length === 0) {
                                return null;
                            }
                            if (settings.length === 1) {
                                return settings[0];
                            }
                            return settings[settingsComboBox.currentIndex];
                        }

                        Connections {
                            target: modelData
                            function onConnectionFailed(reason) {
                                failLoader.sourceComponent = failComponent;
                                failLoader.item.failReason = reason;
                            }
                            function onStateChanged() {
                                if (modelData.state == ConnectionState.Connecting) {
                                    failLoader.sourceComponent = null;
                                }
                            }
                        }

                        Component {
                            id: failComponent
                            RowLayout {
                                property var failReason
                                Label {
                                    text: ConnectionFailReason.toString(failReason)
                                }
                                RowLayout {
                                    TextField {
                                        id: pskField
                                        placeholderText: "PSK"
                                    }
                                    Button {
                                        text: "Set"
                                        visible: pskField.visible
                                        onClicked: {
                                            modelData.connectWithPsk(pskField.text);
                                            failLoader.sourceComponent = null;
                                        }
                                    }
                                    visible: modelData.security === WifiSecurityType.WpaPsk || modelData.security === WifiSecurityType.Wpa2Psk || modelData.security === WifiSecurityType.Sae
                                }
                                Button {
                                    text: "Close"
                                    onClicked: failLoader.sourceComponent = null
                                }
                            }
                        }

                        Layout.fillWidth: true
                        color: modelData.connected ? palette.highlight : palette.button
                        border.color: palette.mid
                        border.width: 1
                        margin: 5

                        ColumnLayout {
                            Layout.fillWidth: true

                            RowLayout {
                                Layout.fillWidth: true
                                Label {
                                    text: modelData.name
                                    font.bold: true
                                }
                                Label {
                                    visible: modelData.device?.type === DeviceType.Wifi
                                    text: `${Math.round(modelData.signalStrength * 100)}%`
                                    color: palette.placeholderText
                                }
                                Label {
                                    visible: modelData.device?.type === DeviceType.Wifi
                                    text: WifiSecurityType.toString(modelData.security)
                                    color: palette.placeholderText
                                }
                                Label {
                                    text: modelData.known ? "Known" : ""
                                    color: palette.placeholderText
                                }
                                Item { Layout.fillWidth: true }
                                RowLayout {
                                    Layout.alignment: Qt.AlignRight
                                    BusyIndicator {
                                        implicitHeight: 30
                                        implicitWidth: 30
                                        running: modelData.stateChanging
                                        visible: modelData.stateChanging
                                    }
                                    Label {
                                        text: ConnectionState.toString(modelData.state)
                                        color: modelData.connected ? palette.link : palette.placeholderText
                                    }
                                    Button {
                                        text: "Connect"
                                        onClicked: {
                                            if (ethernetNetwork.chosenSettings)
                                                modelData.connectWithSettings(ethernetNetwork.chosenSettings);
                                            else
                                                modelData.connect();
                                        }
                                        visible: !modelData.connected
                                    }
                                    Button {
                                        text: "Disconnect"
                                        onClicked: modelData.disconnect()
                                        visible: modelData.connected
                                    }
                                    Button {
                                        text: "Forget"
                                        onClicked: modelData.forget()
                                        visible: modelData.known
                                    }
                                    Button {
                                        text: "Edit"
                                        visible: modelData.known
                                        onClicked: {
                                            if (ethernetNetwork.chosenSettings)
                                                editorComponent.createObject(null, {
                                                    nmSettings: chosenSettings
                                                });
                                        }
                                    }
                                    Button {
                                        id: detailsButton
                                        text: checked ? "Hide Details" : "Details"
                                        checkable: true
                                    }
                                }
                            }

                            Loader {
                                id: failLoader
                                Layout.alignment: Qt.AlignRight
                                visible: sourceComponent !== null
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                visible: detailsButton.checked
                                spacing: 5

                                RowLayout {
                                    Layout.fillWidth: true
                                    Item { Layout.fillWidth: true }
                                    RowLayout {
                                        visible: modelData.nmSettings.length > 1
                                        Label { text: "Choose settings:" }
                                        ComboBox {
                                            id: settingsComboBox
                                            model: modelData.nmSettings.map(s => s?.read()?.connection?.id)
                                            currentIndex: 0
                                        }
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    visible: modelData.device?.type === DeviceType.Wifi
                                    spacing: 5

                                    Label {
                                        text: "Access Points"
                                        font.bold: true
                                    }

                                    GridLayout {
                                        id: apTable
                                        Layout.fillWidth: true
                                        columns: 5
                                        columnSpacing: 12
                                        rowSpacing: 5

                                        readonly property var accessPoints: modelData.accessPoints?.values ?? []

                                        Label {
                                            Layout.fillWidth: true
                                            text: "BSSID"
                                            color: palette.placeholderText
                                        }
                                        Label {
                                            text: "Signal"
                                            color: palette.placeholderText
                                        }
                                        Label {
                                            text: "Frequency"
                                            color: palette.placeholderText
                                        }
                                        Label {
                                            text: "Security"
                                            color: palette.placeholderText
                                        }
                                        Label {
                                            text: "Active"
                                            color: palette.placeholderText
                                        }

                                        // Cells share one grid so headers and rows use the same column widths.
                                        Repeater {
                                            model: apTable.accessPoints.length * apTable.columns

                                            Loader {
                                                id: apCell
                                                required property int index
                                                readonly property int column: index % apTable.columns
                                                readonly property AccessPoint ap: apTable.accessPoints[Math.floor(index / apTable.columns)] ?? null
                                                Layout.fillWidth: column === 0
                                                sourceComponent: column === 4 ? activeCell : textCell

                                                Component {
                                                    id: textCell
                                                    Label {
                                                        text: {
                                                            if (!apCell.ap) return "";
                                                            switch (apCell.column) {
                                                            case 0: return apCell.ap.bssid;
                                                            case 1: return `${Math.round(apCell.ap.signalStrength * 100)}%`;
                                                            case 2: return `${apCell.ap.frequency} MHz`;
                                                            case 3: return WifiSecurityType.toString(apCell.ap.security);
                                                            }
                                                        }
                                                    }
                                                }
                                                Component {
                                                    id: activeCell
                                                    CheckBox {
                                                        enabled: false
                                                        checked: apCell.ap?.active ?? false
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    Label {
                                        text: "No access points available"
                                        visible: !modelData.accessPoints?.values.length
                                        color: palette.placeholderText
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    FloatingWindow {
        color: contentItem.palette.window

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 5

            ColumnLayout {
                Label {
                    text: `Networking (${NetworkBackendType.toString(Networking.backend)} backend)`
                    font.bold: true
                    font.pointSize: 12
                }
                RowLayout {
                    Label {
                        text: `Connectivity`
                        font.bold: true
                    }
                    Label {
                        text: `${NetworkConnectivity.toString(Networking.connectivity)}`
                        visible: Networking.canCheckConnectivity
                    }
                    Button {
                        text: "Re-check"
                        visible: Networking.canCheckConnectivity && Networking.connectivityCheckEnabled
                        onClicked: Networking.checkConnectivity()
                    }
                    CheckBox {
                        text: "Checking enabled"
                        checked: Networking.connectivityCheckEnabled
                        onClicked: Networking.connectivityCheckEnabled = !Networking.connectivityCheckEnabled
                        visible: Networking.canCheckConnectivity
                    }
                    CheckBox {
                        enabled: false
                        text: "Supported"
                        checked: Networking.canCheckConnectivity
                    }
                }
            }

            Column {
                Layout.fillWidth: true
                RowLayout {
                    Label {
                        text: "WiFi"
                        font.bold: true
                    }
                    CheckBox {
                        text: "Software"
                        checked: Networking.wifiEnabled
                        onClicked: Networking.wifiEnabled = !Networking.wifiEnabled
                    }
                    CheckBox {
                        enabled: false
                        text: "Hardware"
                        checked: Networking.wifiHardwareEnabled
                    }
                }
            }

            ListView {
                clip: true
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: Networking.devices
                delegate: Component {
                    Loader {
                        required property var modelData
                        width: ListView.view.width
                        sourceComponent: deviceDelegate
                    }
                }
            }
        }
    }
}
