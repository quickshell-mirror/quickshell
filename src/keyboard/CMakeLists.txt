find_package(PkgConfig REQUIRED)
pkg_check_modules(xkbregistry REQUIRED IMPORTED_TARGET xkbregistry)

qt_add_library(quickshell-keyboard STATIC
	keyboard_layout.cpp
)

qt_add_qml_module(quickshell-keyboard
	URI Quickshell.Keyboard
	VERSION 0.1
	DEPENDENCIES QtQuick
)

qs_add_module_deps_light(quickshell-keyboard Quickshell)

install_qml_module(quickshell-keyboard)

target_link_libraries(quickshell-keyboard PRIVATE Qt::Quick PkgConfig::xkbregistry)

qs_module_pch(quickshell-keyboard)

target_link_libraries(quickshell PRIVATE quickshell-keyboardplugin)
