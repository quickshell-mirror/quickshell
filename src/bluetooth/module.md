name = "Quickshell.Bluetooth"
description = "Bluetooth API"
headers = [
	"bluez.hpp",
	"adapter.hpp",
	"device.hpp",
]
-----
This module exposes Bluetooth management APIs provided by the BlueZ DBus interface.
Both DBus and BlueZ must be running to use it.

See the @@Quickshell.Bluetooth.Bluetooth singleton.
