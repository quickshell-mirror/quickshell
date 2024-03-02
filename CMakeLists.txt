cmake_minimum_required(VERSION 3.20)
project(quickshell VERSION "0.0.1")

set(QT_MIN_VERSION "6.6.0")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(TESTS "Build tests" OFF)

option(SOCKETS "Enable unix socket support" ON)
option(WAYLAND "Enable wayland support" ON)
option(WAYLAND_WLR_LAYERSHELL "Support the zwlr_layer_shell_v1 wayland protocol" ON)
option(WAYLAND_SESSION_LOCK "Support the ext_session_lock_v1 wayland protocol" ON)

message(STATUS "Quickshell configuration")
message(STATUS "  Build tests: ${TESTS}")
message(STATUS "  Sockets: ${SOCKETS}")
message(STATUS "  Wayland: ${WAYLAND}")
if (WAYLAND)
	message(STATUS "    Wlroots Layershell: ${WAYLAND_WLR_LAYERSHELL}")
	message(STATUS "    Session Lock: ${WAYLAND_SESSION_LOCK}")
endif ()

add_compile_options(-Wall -Wextra)

# nix workaround
if (CMAKE_EXPORT_COMPILE_COMMANDS)
	set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

if (NOT CMAKE_BUILD_TYPE)
	message(STATUS "CMAKE_BUILD_TYPE unset, defaulting to Debug.")
	set(CMAKE_BUILD_TYPE Debug)
endif()

set(QT_DEPS Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2)
set(QT_FPDEPS Gui Qml Quick QuickControls2)

if (TESTS)
	enable_testing()
	list(APPEND QT_FPDEPS Test)
endif()

if (SOCKETS)
	list(APPEND QT_DEPS Qt6::Network)
	list(APPEND QT_FPDEPS Network)
endif()

if (WAYLAND)
	list(APPEND QT_DEPS Qt6::WaylandClient Qt6::WaylandClientPrivate)
	list(APPEND QT_FPDEPS WaylandClient)
endif()

find_package(Qt6 REQUIRED COMPONENTS ${QT_FPDEPS})

qt_standard_project_setup(REQUIRES 6.6)

add_subdirectory(src/core)

if (WAYLAND)
	add_subdirectory(src/wayland)
endif ()
