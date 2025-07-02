qt_add_library(quickshell-hyprland-ipc STATIC
	connection.cpp
	monitor.cpp
	workspace.cpp
	qml.cpp
)

qt_add_qml_module(quickshell-hyprland-ipc
	URI Quickshell.Hyprland._Ipc
	VERSION 0.1
	DEPENDENCIES QtQuick
)

qs_add_module_deps_light(quickshell-hyprland-ipc Quickshell)

install_qml_module(quickshell-hyprland-ipc)

target_link_libraries(quickshell-hyprland-ipc PRIVATE Qt::Quick)

if (WAYLAND_TOPLEVEL_MANAGEMENT)
	target_sources(quickshell-hyprland-ipc PRIVATE
		toplevel_mapping.cpp
		hyprland_toplevel.cpp
	)

	wl_proto(wlp-hyprland-toplevel-mapping hyprland-toplevel-mapping-v1 "${CMAKE_CURRENT_SOURCE_DIR}")

	target_link_libraries(quickshell-hyprland-ipc PRIVATE
		Qt::WaylandClient Qt::WaylandClientPrivate wayland-client
		wlp-hyprland-toplevel-mapping
		wlp-foreign-toplevel
	)
endif()

qs_module_pch(quickshell-hyprland-ipc SET large)

target_link_libraries(quickshell PRIVATE quickshell-hyprland-ipcplugin)
