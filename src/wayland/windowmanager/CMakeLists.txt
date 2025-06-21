qt_add_library(quickshell-wayland-windowsystem STATIC
	windowmanager.cpp
	workspace.cpp
	ext_workspace.cpp
)

add_library(quickshell-wayland-windowsystem-init OBJECT init.cpp)
target_link_libraries(quickshell-wayland-windowsystem-init PRIVATE Qt::Quick)

#wl_proto(wlp-ext-foreign-toplevel ext-foreign-toplevel-list-v1 "${WAYLAND_PROTOCOLS}/staging/ext-foreign-toplevel-list")
wl_proto(wlp-ext-workspace ext-workspace-v1 "${WAYLAND_PROTOCOLS}/staging/ext-workspace")

target_link_libraries(quickshell-wayland-windowsystem PRIVATE
	Qt::WaylandClient Qt::WaylandClientPrivate wayland-client
	Qt::Quick # for pch? potentially, check w/ gcc

	wlp-ext-foreign-toplevel wlp-ext-workspace
)

target_link_libraries(quickshell PRIVATE quickshell-wayland-windowsystem quickshell-wayland-windowsystem-init)
