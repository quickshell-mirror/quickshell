qt_add_library(quickshell-wayland-windowsystem STATIC
	windowmanager.cpp
	windowset.cpp
	ext_workspace.cpp
)

add_library(quickshell-wayland-windowsystem-init OBJECT init.cpp)
target_link_libraries(quickshell-wayland-windowsystem-init PRIVATE Qt::Quick)

wl_proto(wlp-ext-workspace ext-workspace-v1 "${WAYLAND_PROTOCOLS}/staging/ext-workspace")

target_link_libraries(quickshell-wayland-windowsystem PRIVATE
	Qt::Quick Qt::WaylandClient Qt::WaylandClientPrivate wayland-client
	wlp-ext-workspace
)

qs_pch(quickshell-wayland-windowsystem SET large)

target_link_libraries(quickshell PRIVATE quickshell-wayland-windowsystem quickshell-wayland-windowsystem-init)
