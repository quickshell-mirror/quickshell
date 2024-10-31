qt_add_library(quickshell-hyprland-focus-grab STATIC
	manager.cpp
	grab.cpp
	qml.cpp
)

qt_add_qml_module(quickshell-hyprland-focus-grab
	URI Quickshell.Hyprland._FocusGrab
	VERSION 0.1
	DEPENDENCIES QtQml Quickshell
)

wl_proto(quickshell-hyprland-focus-grab
	hyprland-focus-grab-v1
	"${CMAKE_CURRENT_SOURCE_DIR}/hyprland-focus-grab-v1.xml"
)

target_link_libraries(quickshell-hyprland-focus-grab PRIVATE ${QT_DEPS} wayland-client)

qs_pch(quickshell-hyprland-focus-grab)
qs_pch(quickshell-hyprland-focus-grabplugin)

target_link_libraries(quickshell PRIVATE quickshell-hyprland-focus-grabplugin)
