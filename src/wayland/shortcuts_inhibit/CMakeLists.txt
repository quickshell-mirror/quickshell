qt_add_library(quickshell-wayland-shortcuts-inhibit STATIC
	proto.cpp
	inhibitor.cpp
)

qt_add_qml_module(quickshell-wayland-shortcuts-inhibit
	URI Quickshell.Wayland._ShortcutsInhibitor
	VERSION 0.1
	DEPENDENCIES QtQuick
)

install_qml_module(quickshell-wayland-shortcuts-inhibit)

qs_add_module_deps_light(quickshell-wayland-shortcuts-inhibit Quickshell)

wl_proto(wlp-shortcuts-inhibit keyboard-shortcuts-inhibit-unstable-v1 "${WAYLAND_PROTOCOLS}/unstable/keyboard-shortcuts-inhibit")

target_link_libraries(quickshell-wayland-shortcuts-inhibit PRIVATE
	Qt::Quick Qt::WaylandClient Qt::WaylandClientPrivate wayland-client
	wlp-shortcuts-inhibit
)

qs_module_pch(quickshell-wayland-shortcuts-inhibit SET large)

target_link_libraries(quickshell PRIVATE quickshell-wayland-shortcuts-inhibitplugin)