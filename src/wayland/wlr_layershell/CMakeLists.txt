qt_add_library(quickshell-wayland-layershell STATIC
	shell_integration.cpp
	surface.cpp
	window.cpp
)

qt_add_qml_module(quickshell-wayland-layershell
	URI Quickshell.Wayland._WlrLayerShell
	VERSION 0.1
	DEPENDENCIES QtQuick
)

qs_add_module_deps_light(quickshell-wayland-layershell Quickshell Quickshell.Wayland)

install_qml_module(quickshell-wayland-layershell)

wl_proto(quickshell-wayland-layershell wlr-layer-shell-unstable-v1 "${CMAKE_CURRENT_SOURCE_DIR}/wlr-layer-shell-unstable-v1.xml")

# link dependency of wlr-layer-shell's codegen
wl_proto(quickshell-wayland-layershell xdg-shell "${WAYLAND_PROTOCOLS}/stable/xdg-shell/xdg-shell.xml")

target_link_libraries(quickshell-wayland-layershell PRIVATE
	Qt::Quick Qt::WaylandClient Qt::WaylandClientPrivate wayland-client
)

qs_module_pch(quickshell-wayland-layershell SET large)

target_link_libraries(quickshell-wayland PRIVATE quickshell-wayland-layershellplugin)
