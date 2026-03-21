qt_add_library(quickshell-windowmanager STATIC
	screenprojection.cpp
	windowmanager.cpp
	windowset.cpp
)

qt_add_qml_module(quickshell-windowmanager
	URI Quickshell.WindowManager
	VERSION 0.1
	DEPENDENCIES QtQuick
)

qs_add_module_deps_light(quickshell-windowmanager Quickshell)

install_qml_module(quickshell-windowmanager)

qs_module_pch(quickshell-windowmanager SET large)

target_link_libraries(quickshell-windowmanager PRIVATE Qt::Quick)
target_link_libraries(quickshell PRIVATE quickshell-windowmanagerplugin)
