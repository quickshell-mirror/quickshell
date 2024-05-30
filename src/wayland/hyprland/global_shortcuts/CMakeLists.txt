qt_add_library(quickshell-hyprland-global-shortcuts STATIC
	qml.cpp
	manager.cpp
	shortcut.cpp
)

qt_add_qml_module(quickshell-hyprland-global-shortcuts
	URI Quickshell.Hyprland._GlobalShortcuts
	VERSION 0.1
)

wl_proto(quickshell-hyprland-global-shortcuts
	hyprland-global-shortcuts-v1
	"${CMAKE_CURRENT_SOURCE_DIR}/hyprland-global-shortcuts-v1.xml"
)

target_link_libraries(quickshell-hyprland-global-shortcuts PRIVATE ${QT_DEPS} wayland-client)

qs_pch(quickshell-hyprland-global-shortcuts)
qs_pch(quickshell-hyprland-global-shortcutsplugin)

target_link_libraries(quickshell PRIVATE quickshell-hyprland-global-shortcutsplugin)
