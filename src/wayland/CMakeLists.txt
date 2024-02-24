qt_add_library(quickshell-wayland STATIC
	shell_integration.cpp
	layer_surface.cpp
	layershell.cpp
	waylandshellwindow.cpp
)

qt_add_qml_module(quickshell-wayland URI QuickShell.Wayland)

find_package(PkgConfig REQUIRED)
pkg_check_modules(wayland REQUIRED IMPORTED_TARGET wayland-client wayland-protocols)

find_package(Qt6 REQUIRED COMPONENTS WaylandClient)

target_link_libraries(quickshell-wayland PRIVATE ${QT_DEPS} wayland-client)
message(STATUS Qt6::WaylandClient)

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
	OUTPUT_VARIABLE WAYLAND_PROTOCOLS_DIR
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Found wayland-protocols at ${WAYLAND_PROTOCOLS_DIR}")

set(PROTO_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/wl-proto)
set(PROTO_BUILD_PATH ${CMAKE_CURRENT_BINARY_DIR}/wl-proto)
make_directory(${PROTO_BUILD_PATH})

function (wl_proto name path)
	execute_process(COMMAND ${waylandscanner} client-header ${path} ${PROTO_BUILD_PATH}/wayland-${name}-client-protocol.h)
	execute_process(COMMAND ${waylandscanner} private-code ${path} ${PROTO_BUILD_PATH}/wayland-${name}.c)
	execute_process(COMMAND ${qtwaylandscanner} client-header ${path} OUTPUT_FILE ${PROTO_BUILD_PATH}/qwayland-${name}.h)
	execute_process(COMMAND ${qtwaylandscanner} client-code ${path} OUTPUT_FILE ${PROTO_BUILD_PATH}/qwayland-${name}.cpp)

	target_sources(quickshell-wayland PRIVATE ${PROTO_BUILD_PATH}/wayland-${name}.c)
	target_sources(quickshell-wayland PRIVATE ${PROTO_BUILD_PATH}/qwayland-${name}.cpp)
endfunction()

wl_proto("wlr-layer-shell-unstable-v1" "${PROTO_SRC_PATH}/wlr-layer-shell-unstable-v1.xml")

target_include_directories(quickshell-wayland PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/wl-proto)
