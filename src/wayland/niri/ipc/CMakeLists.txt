qt_add_library(quickshell-niri-ipc STATIC
	connection.cpp
	output.cpp
	workspace.cpp
	window.cpp
	qml.cpp
)

qt_add_qml_module(quickshell-niri-ipc
	URI Quickshell.Niri._Ipc
	VERSION 0.1
	DEPENDENCIES QtQuick
)

qs_add_module_deps_light(quickshell-niri-ipc Quickshell)

install_qml_module(quickshell-niri-ipc)

target_link_libraries(quickshell-niri-ipc PRIVATE Qt::Quick Qt::Network)

qs_module_pch(quickshell-niri-ipc SET large)

target_link_libraries(quickshell PRIVATE quickshell-niri-ipcplugin)
