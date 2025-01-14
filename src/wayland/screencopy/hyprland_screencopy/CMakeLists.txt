qt_add_library(quickshell-wayland-screencopy-hyprland STATIC
	hyprland_screencopy.cpp
)

wl_proto(wlp-hyprland-screencopy hyprland-toplevel-export-v1 "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(quickshell-wayland-screencopy-hyprland PRIVATE
	Qt::WaylandClient Qt::WaylandClientPrivate wayland-client
	Qt::Quick # for pch
)

target_link_libraries(quickshell-wayland-screencopy-hyprland PUBLIC
	wlp-hyprland-screencopy wlp-foreign-toplevel
)

qs_pch(quickshell-wayland-screencopy-hyprland SET large)
