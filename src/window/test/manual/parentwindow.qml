import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Fusion
import Quickshell

Scope {
    FloatingWindow {
        id: control
        color: contentItem.palette.window
        ColumnLayout {
            CheckBox {
                id: parentCb
                text: "Show parent"
            }

            CheckBox {
                id: dialogCb
                text: "Show dialog"
            }
        }
    }

    FloatingWindow {
        id: parentw
        Text {
            text: "parent"
        }
        visible: parentCb.checked
        color: contentItem.palette.window

        FloatingWindow {
            id: dialog
            parentWindow: parentw
            visible: dialogCb.checked
            color: contentItem.palette.window

            Text {
                text: "dialog"
            }
        }
    }
}
