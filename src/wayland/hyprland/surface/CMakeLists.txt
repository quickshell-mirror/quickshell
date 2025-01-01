qt_add_library(quickshell-hyprland-surface-extensions STATIC
	qml.cpp
	manager.cpp
	surface.cpp
)

qt_add_qml_module(quickshell-hyprland-surface-extensions
	URI Quickshell.Hyprland._SurfaceExtensions
	VERSION 0.1
	DEPENDENCIES QtQml
)

install_qml_module(quickshell-hyprland-surface-extensions)

wl_proto(quickshell-hyprland-surface-extensions
	hyprland-surface-v1
	"${CMAKE_CURRENT_SOURCE_DIR}/hyprland-surface-v1.xml"
)

target_link_libraries(quickshell-hyprland-surface-extensions PRIVATE
	Qt::Quick Qt::WaylandClient Qt::WaylandClientPrivate wayland-client
)

qs_module_pch(quickshell-hyprland-surface-extensions)

target_link_libraries(quickshell PRIVATE quickshell-hyprland-surface-extensionsplugin)
