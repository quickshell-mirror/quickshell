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

qs_add_pchset(wayland-protocol
	DEPENDENCIES Qt::Core Qt::WaylandClient Qt::WaylandClientPrivate
	HEADERS
		<wayland-client.h>
		<qbytearray.h>
		<qstring.h>
)

function (wl_proto target name dir)
	set(PROTO_BUILD_PATH ${CMAKE_CURRENT_BINARY_DIR}/wl-proto/${name})
	make_directory(${PROTO_BUILD_PATH})

	set(WS_CLIENT_HEADER "${PROTO_BUILD_PATH}/wayland-${name}-client-protocol.h")
	set(WS_CLIENT_CODE "${PROTO_BUILD_PATH}/wayland-${name}.c")
	set(QWS_CLIENT_HEADER "${PROTO_BUILD_PATH}/qwayland-${name}.h")
	set(QWS_CLIENT_CODE "${PROTO_BUILD_PATH}/qwayland-${name}.cpp")
	set(PATH "${dir}/${name}.xml")

	add_custom_command(
		OUTPUT "${WS_CLIENT_HEADER}"
		COMMAND Wayland::Scanner client-header "${PATH}" "${WS_CLIENT_HEADER}"
		DEPENDS Wayland::Scanner "${PATH}"
	)

	add_custom_command(
		OUTPUT "${WS_CLIENT_CODE}"
		COMMAND Wayland::Scanner private-code "${PATH}" "${WS_CLIENT_CODE}"
		DEPENDS Wayland::Scanner "${PATH}"
	)

	add_custom_command(
		OUTPUT "${QWS_CLIENT_HEADER}"
		COMMAND Qt6::qtwaylandscanner client-header "${PATH}" > "${QWS_CLIENT_HEADER}"
		DEPENDS Qt6::qtwaylandscanner "${PATH}"
	)

	add_custom_command(
		OUTPUT "${QWS_CLIENT_CODE}"
		COMMAND Qt6::qtwaylandscanner client-code "${PATH}" > "${QWS_CLIENT_CODE}"
		DEPENDS Qt6::qtwaylandscanner "${PATH}"
	)

	add_library(wl-proto-${name}-wl STATIC ${WS_CLIENT_HEADER} ${WS_CLIENT_CODE})
	add_library(${target} STATIC ${QWS_CLIENT_HEADER} ${QWS_CLIENT_CODE})

	target_include_directories(${target} INTERFACE ${PROTO_BUILD_PATH})
	target_link_libraries(${target} wl-proto-${name}-wl Qt6::WaylandClient Qt6::WaylandClientPrivate)
	qs_pch(${target} SET wayland-protocol)
endfunction()

# -----

qt_add_library(quickshell-wayland STATIC
	platformmenu.cpp
	popupanchor.cpp
	xdgshell.cpp
	util.cpp
	output_tracking.cpp
)

# required to make sure the constructor is linked
add_library(quickshell-wayland-init OBJECT init.cpp)

set(WAYLAND_MODULES)

if (WAYLAND_WLR_LAYERSHELL)
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

if (SCREENCOPY)
	add_subdirectory(buffer)
	add_subdirectory(screencopy)
	list(APPEND WAYLAND_MODULES Quickshell.Wayland._Screencopy)
endif()


if (HYPRLAND)
	add_subdirectory(hyprland)
endif()

# widgets for qmenu
target_link_libraries(quickshell-wayland PRIVATE
	Qt::Quick Qt::Widgets Qt::WaylandClient Qt::WaylandClientPrivate
)

target_link_libraries(quickshell-wayland-init PRIVATE Qt::Quick)

qt_add_qml_module(quickshell-wayland
	URI Quickshell.Wayland
	VERSION 0.1
	DEPENDENCIES QtQuick
	IMPORTS ${WAYLAND_MODULES}
)

qs_add_module_deps_light(quickshell-wayland Quickshell)

install_qml_module(quickshell-wayland)

qs_module_pch(quickshell-wayland SET large)

target_link_libraries(quickshell PRIVATE quickshell-waylandplugin quickshell-wayland-init)
