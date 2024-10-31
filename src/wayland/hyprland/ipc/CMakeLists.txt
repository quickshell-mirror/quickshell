qt_add_library(quickshell-hyprland-ipc STATIC
	connection.cpp
	monitor.cpp
	workspace.cpp
	qml.cpp
)

qt_add_qml_module(quickshell-hyprland-ipc
	URI Quickshell.Hyprland._Ipc
	VERSION 0.1
	DEPENDENCIES QtQml Quickshell
)

target_link_libraries(quickshell-hyprland-ipc PRIVATE ${QT_DEPS})

qs_pch(quickshell-hyprland-ipc)
qs_pch(quickshell-hyprland-ipcplugin)

target_link_libraries(quickshell PRIVATE quickshell-hyprland-ipcplugin)
