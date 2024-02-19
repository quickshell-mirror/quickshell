cmake_minimum_required(VERSION 3.20)
project(quickshell VERSION "0.0.1")

set(QT_MIN_VERSION "6.6.0")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(LAYERSHELL "Enable wayland layershell support" ON)

add_compile_options(-Wall -Wextra)

# nix workaround
if (CMAKE_EXPORT_COMPILE_COMMANDS)
	set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

if (NOT CMAKE_BUILD_TYPE)
	message(STATUS "CMAKE_BUILD_TYPE unset, defaulting to Debug.")
	set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(Qt6 REQUIRED COMPONENTS Gui Qml Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.6)

add_subdirectory(src/core)
