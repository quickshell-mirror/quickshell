name = "Quickshell.Keyboard"
description = "Compositor-agnostic keyboard layout types"
headers = [
	"keyboard_layout.hpp",
]
-----
Provides xkbcommon-based keyboard layout data that can be used by
any compositor backend to expose active and available layouts.

KeyboardLayout objects are obtained through compositor-specific
types such as @@HyprlandKeyboards.
