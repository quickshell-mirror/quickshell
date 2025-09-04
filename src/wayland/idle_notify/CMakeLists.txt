qt_add_library(quickshell-wayland-idle-notify STATIC
	proto.cpp
	monitor.cpp
)

qt_add_qml_module(quickshell-wayland-idle-notify
	URI Quickshell.Wayland._IdleNotify
	VERSION 0.1
	DEPENDENCIES QtQuick
)

install_qml_module(quickshell-wayland-idle-notify)

qs_add_module_deps_light(quickshell-wayland-idle-notify Quickshell)

wl_proto(wlp-idle-notify ext-idle-notify-v1 "${WAYLAND_PROTOCOLS}/staging/ext-idle-notify")

target_link_libraries(quickshell-wayland-idle-notify PRIVATE
	Qt::Quick Qt::WaylandClient Qt::WaylandClientPrivate wayland-client
	wlp-idle-notify
)

qs_module_pch(quickshell-wayland-idle-notify SET large)

target_link_libraries(quickshell PRIVATE quickshell-wayland-idle-notifyplugin)
