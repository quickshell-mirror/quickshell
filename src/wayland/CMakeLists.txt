find_package(PkgConfig REQUIRED)
pkg_check_modules(wayland REQUIRED IMPORTED_TARGET wayland-client wayland-protocols)

find_package(Qt6 REQUIRED COMPONENTS WaylandClient)

# wayland protocols

if (DEFINED ENV{QTWAYLANDSCANNER})
	set(qtwaylandscanner $ENV{QTWAYLANDSCANNER})
else()
	find_program(qtwaylandscanner NAMES qtwaylandscanner)
endif()

if (qtwaylandscanner STREQUAL "qtwaylandscanner-NOTFOUND")
	message(FATAL_ERROR "qtwaylandscanner not found. Set the QTWAYLANDSCANNER environment variable to specify its path explicity.")
endif()

message(STATUS "Found qtwaylandscanner at ${qtwaylandscanner}")

find_program(waylandscanner NAMES wayland-scanner)
message(STATUS "Found wayland-scanner at ${waylandscanner}")

execute_process(
	COMMAND pkg-config --variable=pkgdatadir wayland-protocols
	OUTPUT_VARIABLE WAYLAND_PROTOCOLS
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Found wayland-protocols at ${WAYLAND_PROTOCOLS_DIR}")

function (wl_proto target name path)
	set(PROTO_BUILD_PATH ${CMAKE_CURRENT_BINARY_DIR}/wl-proto/${name})
	make_directory(${PROTO_BUILD_PATH})

	execute_process(COMMAND ${waylandscanner} client-header ${path} ${PROTO_BUILD_PATH}/wayland-${name}-client-protocol.h)
	execute_process(COMMAND ${waylandscanner} private-code ${path} ${PROTO_BUILD_PATH}/wayland-${name}.c)
	execute_process(COMMAND ${qtwaylandscanner} client-header ${path} OUTPUT_FILE ${PROTO_BUILD_PATH}/qwayland-${name}.h)
	execute_process(COMMAND ${qtwaylandscanner} client-code ${path} OUTPUT_FILE ${PROTO_BUILD_PATH}/qwayland-${name}.cpp)

	add_library(wl-proto-${name}
		${PROTO_BUILD_PATH}/wayland-${name}.c
		${PROTO_BUILD_PATH}/qwayland-${name}.cpp
	)

	target_include_directories(wl-proto-${name} INTERFACE ${PROTO_BUILD_PATH})
	target_link_libraries(wl-proto-${name} Qt6::WaylandClient Qt6::WaylandClientPrivate)

	target_link_libraries(${target} PRIVATE wl-proto-${name})
endfunction()

# -----

qt_add_library(quickshell-wayland STATIC)

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
