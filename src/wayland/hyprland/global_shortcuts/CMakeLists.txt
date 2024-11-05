qt_add_library(quickshell-hyprland-global-shortcuts STATIC
	qml.cpp
	manager.cpp
	shortcut.cpp
)

qt_add_qml_module(quickshell-hyprland-global-shortcuts
	URI Quickshell.Hyprland._GlobalShortcuts
	VERSION 0.1
	DEPENDENCIES QtQml
)

install_qml_module(quickshell-hyprland-global-shortcuts)

wl_proto(quickshell-hyprland-global-shortcuts
	hyprland-global-shortcuts-v1
	"${CMAKE_CURRENT_SOURCE_DIR}/hyprland-global-shortcuts-v1.xml"
)

target_link_libraries(quickshell-hyprland-global-shortcuts PRIVATE
	Qt::Qml Qt::WaylandClient Qt::WaylandClientPrivate wayland-client
)

qs_module_pch(quickshell-hyprland-global-shortcuts)

target_link_libraries(quickshell PRIVATE quickshell-hyprland-global-shortcutsplugin)
