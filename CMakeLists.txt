cmake_minimum_required(VERSION 3.20)
project(quickshell VERSION "0.0.1")

set(QT_MIN_VERSION "6.6.0")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(WAYLAND "Enable wayland support" ON)

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

if (WAYLAND)
	list(APPEND QT_DEPS Qt6::WaylandClient Qt6::WaylandClientPrivate)
	list(APPEND QT_FPDEPS WaylandClient)
endif()

find_package(Qt6 REQUIRED COMPONENTS ${QT_FPDEPS})

qt_standard_project_setup(REQUIRES 6.6)

add_subdirectory(src/core)
add_subdirectory(src/wayland)
