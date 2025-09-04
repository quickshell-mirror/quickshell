qt_add_library(quickshell-wayland-idle-inhibit STATIC
	proto.cpp
	inhibitor.cpp
)

qt_add_qml_module(quickshell-wayland-idle-inhibit
	URI Quickshell.Wayland._IdleInhibitor
	VERSION 0.1
	DEPENDENCIES QtQuick
)

install_qml_module(quickshell-wayland-idle-inhibit)

qs_add_module_deps_light(quickshell-wayland-idle-inhibit Quickshell)

wl_proto(wlp-idle-inhibit idle-inhibit-unstable-v1 "${WAYLAND_PROTOCOLS}/unstable/idle-inhibit")

target_link_libraries(quickshell-wayland-idle-inhibit PRIVATE
	Qt::Quick Qt::WaylandClient Qt::WaylandClientPrivate wayland-client
	wlp-idle-inhibit
)

qs_module_pch(quickshell-wayland-idle-inhibit SET large)

target_link_libraries(quickshell PRIVATE quickshell-wayland-idle-inhibitplugin)
