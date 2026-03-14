qt_add_library(quickshell-wayland-sessionlock STATIC
	manager.cpp
	surface.cpp
	lock.cpp
	shell_integration.cpp
	session_lock.cpp
)
wl_proto(wlp-session-lock ext-session-lock-v1 "${WAYLAND_PROTOCOLS}/staging/ext-session-lock")
target_include_directories(quickshell-wayland-sessionlock PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(quickshell-wayland-sessionlock PRIVATE
	Qt::Quick Qt::WaylandClient Qt::WaylandClientPrivate Qt::DBus wayland-client
	wlp-session-lock
	quickshell-dbus
)
qs_pch(quickshell-wayland-sessionlock SET large)
target_link_libraries(quickshell-wayland PRIVATE quickshell-wayland-sessionlock)
