find_package(PkgConfig REQUIRED)
find_package(WaylandScanner REQUIRED)
pkg_check_modules(wayland REQUIRED IMPORTED_TARGET wayland-client wayland-protocols)

# wayland protocols

if(NOT TARGET Wayland::Scanner)
	message(FATAL_ERROR "Wayland::Scanner target not found. You might be missing the WaylandScanner CMake package.")
endif()

if(NOT TARGET Qt6::qtwaylandscanner)
	message(FATAL_ERROR "qtwaylandscanner executable not found. Most likely there is an issue with your Qt installation.")
endif()

execute_process(
	COMMAND pkg-config --variable=pkgdatadir wayland-protocols
	OUTPUT_VARIABLE WAYLAND_PROTOCOLS
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Found wayland-protocols at ${WAYLAND_PROTOCOLS_DIR}")

function (wl_proto target name path)
	set(PROTO_BUILD_PATH ${CMAKE_CURRENT_BINARY_DIR}/wl-proto/${name})
	make_directory(${PROTO_BUILD_PATH})

	set(WS_CLIENT_HEADER "${PROTO_BUILD_PATH}/wayland-${name}-client-protocol.h")
	set(WS_CLIENT_CODE "${PROTO_BUILD_PATH}/wayland-${name}.c")
	set(QWS_CLIENT_HEADER "${PROTO_BUILD_PATH}/qwayland-${name}.h")
	set(QWS_CLIENT_CODE "${PROTO_BUILD_PATH}/qwayland-${name}.cpp")

	add_custom_command(
		OUTPUT "${WS_CLIENT_HEADER}"
		COMMAND Wayland::Scanner client-header "${path}" "${WS_CLIENT_HEADER}"
		DEPENDS Wayland::Scanner "${path}"
	)

	add_custom_command(
		OUTPUT "${WS_CLIENT_CODE}"
		COMMAND Wayland::Scanner private-code "${path}" "${WS_CLIENT_CODE}"
		DEPENDS Wayland::Scanner "${path}"
	)

	add_custom_command(
		OUTPUT "${QWS_CLIENT_HEADER}"
		COMMAND Qt6::qtwaylandscanner client-header "${path}" > "${QWS_CLIENT_HEADER}"
		DEPENDS Qt6::qtwaylandscanner "${path}"
	)

	add_custom_command(
		OUTPUT "${QWS_CLIENT_CODE}"
		COMMAND Qt6::qtwaylandscanner client-code "${path}" > "${QWS_CLIENT_CODE}"
		DEPENDS Qt6::qtwaylandscanner "${path}"
	)

	add_library(wl-proto-${name}
		${WS_CLIENT_HEADER} ${WS_CLIENT_CODE}
		${QWS_CLIENT_HEADER} ${QWS_CLIENT_CODE}
	)

	target_include_directories(wl-proto-${name} INTERFACE ${PROTO_BUILD_PATH})
	target_link_libraries(wl-proto-${name} Qt6::WaylandClient Qt6::WaylandClientPrivate)

	target_link_libraries(${target} PRIVATE wl-proto-${name})
endfunction()

# -----

qt_add_library(quickshell-wayland STATIC
	platformmenu.cpp
	popupanchor.cpp
	xdgshell.cpp
)

# required to make sure the constructor is linked
add_library(quickshell-wayland-init OBJECT init.cpp)

set(WAYLAND_MODULES)

if (WAYLAND_WLR_LAYERSHELL)
	target_sources(quickshell-wayland PRIVATE wlr_layershell.cpp)
	add_subdirectory(wlr_layershell)
	target_compile_definitions(quickshell-wayland PRIVATE QS_WAYLAND_WLR_LAYERSHELL)
	target_compile_definitions(quickshell-wayland-init PRIVATE QS_WAYLAND_WLR_LAYERSHELL)

	list(APPEND WAYLAND_MODULES Quickshell.Wayland._WlrLayerShell)
endif()

if (WAYLAND_SESSION_LOCK)
	target_sources(quickshell-wayland PRIVATE session_lock.cpp)
	add_subdirectory(session_lock)
endif()

if (WAYLAND_TOPLEVEL_MANAGEMENT)
	add_subdirectory(toplevel_management)
	list(APPEND WAYLAND_MODULES Quickshell.Wayland._ToplevelManagement)
endif()

if (HYPRLAND)
	add_subdirectory(hyprland)
endif()

target_link_libraries(quickshell-wayland PRIVATE ${QT_DEPS})
target_link_libraries(quickshell-wayland-init PRIVATE ${QT_DEPS})

qt_add_qml_module(quickshell-wayland
	URI Quickshell.Wayland
	VERSION 0.1
	IMPORTS ${WAYLAND_MODULES}
)

qs_pch(quickshell-wayland)
qs_pch(quickshell-waylandplugin)
qs_pch(quickshell-wayland-init)

target_link_libraries(quickshell PRIVATE quickshell-waylandplugin quickshell-wayland-init)
