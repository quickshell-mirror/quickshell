cmake_minimum_required(VERSION 3.20)
project(qtshell VERSION "0.0.1")

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

if (LAYERSHELL)
	find_package(LayerShellQt REQUIRED)
endif()

qt_standard_project_setup(REQUIRES 6.6)

qt_add_executable(qtshell
	src/cpp/main.cpp
	src/cpp/shell.cpp
	src/cpp/variants.cpp
	src/cpp/rootwrapper.cpp
	src/cpp/proxywindow.cpp
	src/cpp/scavenge.cpp
	src/cpp/rootwrapper.cpp
	src/cpp/qmlglobal.cpp
	src/cpp/qmlscreen.cpp
	src/cpp/watcher.cpp
)

qt_add_qml_module(qtshell URI QtShell)

# qml type registration requires this
target_include_directories(qtshell PRIVATE src/cpp)

target_link_libraries(qtshell PRIVATE Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2)

if (LAYERSHELL)
	target_link_libraries(qtshell PRIVATE LayerShellQtInterface)
	target_compile_definitions(qtshell PRIVATE CONF_LAYERSHELL)

	target_sources(qtshell PRIVATE src/cpp/layershell.cpp)
endif()
