qt_add_library(quickshell-wayland-sessionlock STATIC
	manager.cpp
	surface.cpp
	lock.cpp
	shell_integration.cpp
	session_lock.cpp
)

wl_proto(quickshell-wayland-sessionlock ext-session-lock-v1 "${WAYLAND_PROTOCOLS}/staging/ext-session-lock/ext-session-lock-v1.xml")

target_link_libraries(quickshell-wayland-sessionlock PRIVATE
	Qt::Quick Qt::WaylandClient Qt::WaylandClientPrivate wayland-client
)

qs_pch(quickshell-wayland-sessionlock SET large)

target_link_libraries(quickshell-wayland PRIVATE quickshell-wayland-sessionlock)
