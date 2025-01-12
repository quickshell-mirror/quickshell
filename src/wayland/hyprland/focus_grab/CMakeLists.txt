qt_add_library(quickshell-hyprland-focus-grab STATIC
	manager.cpp
	grab.cpp
	qml.cpp
)

qt_add_qml_module(quickshell-hyprland-focus-grab
	URI Quickshell.Hyprland._FocusGrab
	VERSION 0.1
	DEPENDENCIES QtQml
)

qs_add_module_deps_light(quickshell-hyprland-focus-grab Quickshell)

install_qml_module(quickshell-hyprland-focus-grab)

wl_proto(wlp-hyprland-focus-grab hyprland-focus-grab-v1 "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(quickshell-hyprland-focus-grab PRIVATE
	Qt::Quick Qt::WaylandClient Qt::WaylandClientPrivate wayland-client
	wlp-hyprland-focus-grab
)

qs_module_pch(quickshell-hyprland-focus-grab SET large)

target_link_libraries(quickshell PRIVATE quickshell-hyprland-focus-grabplugin)
