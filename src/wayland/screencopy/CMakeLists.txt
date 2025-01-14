qt_add_library(quickshell-wayland-screencopy STATIC
	manager.cpp
	view.cpp
)

qt_add_qml_module(quickshell-wayland-screencopy
	URI Quickshell.Wayland._Screencopy
	VERSION 0.1
	DEPENDENCIES QtQuick
)

install_qml_module(quickshell-wayland-screencopy)

set(SCREENCOPY_MODULES)

if (SCREENCOPY_ICC)
	add_subdirectory(image_copy_capture)
	list(APPEND SCREENCOPY_MODULES quickshell-wayland-screencopy-icc)
endif()

if (SCREENCOPY_WLR)
	add_subdirectory(wlr_screencopy)
	list(APPEND SCREENCOPY_MODULES quickshell-wayland-screencopy-wlr)
endif()

if (SCREENCOPY_HYPRLAND_TOPLEVEL)
	add_subdirectory(hyprland_screencopy)
	list(APPEND SCREENCOPY_MODULES quickshell-wayland-screencopy-hyprland)
endif()

configure_file(build.hpp.in build.hpp @ONLY)
target_include_directories(quickshell-wayland-screencopy PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(quickshell-wayland-screencopy PRIVATE
	Qt::Quick Qt::WaylandClient Qt::WaylandClientPrivate wayland-client
	quickshell-wayland-buffer
	${SCREENCOPY_MODULES}
)

qs_module_pch(quickshell-wayland-screencopy SET large)

target_link_libraries(quickshell PRIVATE quickshell-wayland-screencopyplugin)
