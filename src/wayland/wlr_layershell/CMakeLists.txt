qt_add_library(quickshell-wayland-layershell STATIC
	shell_integration.cpp
	surface.cpp
	window.cpp
)

qt_add_qml_module(quickshell-wayland-layershell URI Quickshell.Wayland._WlrLayerShell)

wl_proto(quickshell-wayland-layershell wlr-layer-shell-unstable-v1 "${CMAKE_CURRENT_SOURCE_DIR}/wlr-layer-shell-unstable-v1.xml")
target_link_libraries(quickshell-wayland-layershell PRIVATE ${QT_DEPS} wayland-client)

target_link_libraries(quickshell-wayland PRIVATE quickshell-wayland-layershellplugin)
