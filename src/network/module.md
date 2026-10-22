name = "Quickshell.Networking"
description = "Network API"
headers = [
	"qml.hpp",
	"network.hpp",
	"device.hpp",
	"wifi.hpp",
	"wired.hpp",
	"enums.hpp",
	"known_network.hpp",
]
-----
This module exposes Network management APIs provided by a supported network backend.
For now, the only backend available is the NetworkManager DBus interface.
Both DBus and NetworkManager must be running to use it.

See the @@Quickshell.Networking.Networking singleton.
