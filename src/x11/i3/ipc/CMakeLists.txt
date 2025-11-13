qt_add_library(quickshell-i3-ipc STATIC
	connection.cpp
	qml.cpp
	workspace.cpp
	monitor.cpp
	controller.cpp
	listener.cpp
)

qt_add_qml_module(quickshell-i3-ipc
	URI Quickshell.I3._Ipc
	VERSION 0.1
	DEPENDENCIES QtQml
)

qs_add_module_deps_light(quickshell-i3-ipc Quickshell)

install_qml_module(quickshell-i3-ipc)

target_link_libraries(quickshell-i3-ipc PRIVATE Qt::Quick)

qs_module_pch(quickshell-i3-ipc SET large)

target_link_libraries(quickshell PRIVATE quickshell-i3-ipcplugin)
