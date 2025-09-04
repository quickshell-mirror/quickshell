import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Quickshell
import Quickshell.Wayland

FloatingWindow {
	color: contentItem.palette.window

	IdleMonitor {
		id: monitor
		enabled: enabledCb.checked
		timeout: timeoutSb.value
		respectInhibitors: respectInhibitorsCb.checked
	}

	ColumnLayout {
		Label { text: `Is idle? ${monitor.isIdle}` }

		CheckBox {
			id: enabledCb
			text: "Enabled"
			checked: true
		}

		CheckBox {
			id: respectInhibitorsCb
			text: "Respect Inhibitors"
			checked: true
		}

		RowLayout {
			Label { text: "Timeout" }

			SpinBox {
				id: timeoutSb
				editable: true
				from: 0
				to: 1000
				value: 5
			}
		}
	}
}
