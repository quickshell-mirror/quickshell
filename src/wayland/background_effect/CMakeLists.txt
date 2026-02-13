qt_add_library(quickshell-wayland-background-effect STATIC
	manager.cpp
	surface.cpp
	qml.cpp
)

qt_add_qml_module(quickshell-wayland-background-effect
	URI Quickshell.Wayland._BackgroundEffect
	VERSION 0.1
	DEPENDENCIES QtQml
)

install_qml_module(quickshell-wayland-background-effect)

wl_proto(wlp-background-effect ext-background-effect-v1 "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(quickshell-wayland-background-effect PRIVATE
	Qt::Quick Qt::WaylandClient Qt::WaylandClientPrivate wayland-client
	wlp-background-effect
)

qs_module_pch(quickshell-wayland-background-effect)

target_link_libraries(quickshell PRIVATE quickshell-wayland-background-effectplugin)
