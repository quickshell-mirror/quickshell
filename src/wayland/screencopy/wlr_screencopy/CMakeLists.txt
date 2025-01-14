qt_add_library(quickshell-wayland-screencopy-wlr STATIC
	wlr_screencopy.cpp
)

wl_proto(wlp-wlr-screencopy wlr-screencopy-unstable-v1 "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(quickshell-wayland-screencopy-wlr PRIVATE
	Qt::WaylandClient Qt::WaylandClientPrivate wayland-client
	Qt::Quick # for pch
)

target_link_libraries(quickshell-wayland-screencopy-wlr PUBLIC wlp-wlr-screencopy)

qs_pch(quickshell-wayland-screencopy-wlr SET large)
