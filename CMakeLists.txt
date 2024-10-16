cmake_minimum_required(VERSION 3.20)
project(quickshell VERSION "0.1.0" LANGUAGES CXX C)

set(QT_MIN_VERSION "6.6.0")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QS_BUILD_OPTIONS "")

function(boption VAR NAME DEFAULT)
	cmake_parse_arguments(PARSE_ARGV 3 arg "" "REQUIRES" "")

	option(${VAR} ${NAME} ${DEFAULT})

	set(STATUS "${VAR}_status")
	set(EFFECTIVE "${VAR}_effective")
	set(${STATUS} ${${VAR}})
	set(${EFFECTIVE} ${${VAR}})

	if (${${VAR}} AND DEFINED arg_REQUIRES)
		set(REQUIRED_EFFECTIVE "${arg_REQUIRES}_effective")
		if (NOT ${${REQUIRED_EFFECTIVE}})
			set(${STATUS} "OFF (Requires ${arg_REQUIRES})")
			set(${EFFECTIVE} OFF)
		endif()
	endif()

	set(${EFFECTIVE} "${${EFFECTIVE}}" PARENT_SCOPE)

	message(STATUS "  ${NAME}: ${${STATUS}}")

	string(APPEND QS_BUILD_OPTIONS "\\n  ${NAME}: ${${STATUS}}")
	set(QS_BUILD_OPTIONS "${QS_BUILD_OPTIONS}" PARENT_SCOPE)
endfunction()

set(DISTRIBUTOR "Unset" CACHE STRING "Distributor")
string(APPEND QS_BUILD_OPTIONS "  Distributor: ${DISTRIBUTOR}")

message(STATUS "Quickshell configuration")
message(STATUS "  Distributor: ${DISTRIBUTOR}")
boption(DISTRIBUTOR_DEBUGINFO_AVAILABLE "Distributor provided debuginfo" NO)
boption(NO_PCH "Disable precompild headers (dev)" OFF)
boption(BUILD_TESTING "Build tests (dev)" OFF)
boption(ASAN "ASAN (dev)" OFF) # note: better output with gcc than clang
boption(FRAME_POINTERS "Keep Frame Pointers (dev)" ${ASAN})

boption(CRASH_REPORTER "Crash Handling" ON)
boption(USE_JEMALLOC "Use jemalloc" ON)
boption(SOCKETS "Unix Sockets" ON)
boption(WAYLAND "Wayland" ON)
boption(WAYLAND_WLR_LAYERSHELL "  Wlroots Layer-Shell" ON REQUIRES WAYLAND)
boption(WAYLAND_SESSION_LOCK "  Session Lock" ON REQUIRES WAYLAND)
boption(WAYLAND_TOPLEVEL_MANAGEMENT "  Foreign Toplevel Management" ON REQUIRES WAYLAND)
boption(HYPRLAND "  Hyprland" ON REQUIRES WAYLAND)
boption(HYPRLAND_IPC "    Hyprland IPC" ON REQUIRES HYPRLAND)
boption(HYPRLAND_GLOBAL_SHORTCUTS "    Hyprland Global Shortcuts" ON REQUIRES HYPRLAND)
boption(HYPRLAND_FOCUS_GRAB "    Hyprland Focus Grabbing" ON REQUIRES HYPRLAND)
boption(X11 "X11" ON)
boption(SERVICE_STATUS_NOTIFIER "System Tray" ON)
boption(SERVICE_PIPEWIRE "PipeWire" ON)
boption(SERVICE_MPRIS "Mpris" ON)
boption(SERVICE_PAM "Pam" ON)
boption(SERVICE_GREETD "Greetd" ON)
boption(SERVICE_UPOWER "UPower" ON)
boption(SERVICE_NOTIFICATIONS "Notifications" ON)

if (NOT DEFINED GIT_REVISION)
	execute_process(
		COMMAND git rev-parse HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_REVISION
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endif()

add_compile_options(-Wall -Wextra)

if (FRAME_POINTERS)
	add_compile_options(-fno-omit-frame-pointer)
endif()

if (ASAN)
	add_compile_options(-fsanitize=address)
	add_link_options(-fsanitize=address)
endif()

# nix workaround
if (CMAKE_EXPORT_COMPILE_COMMANDS)
	set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

if (NOT CMAKE_BUILD_TYPE)
	message(STATUS "CMAKE_BUILD_TYPE unset, defaulting to Debug.")
	set(CMAKE_BUILD_TYPE Debug)
endif()

set(QT_DEPS Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Widgets)
set(QT_FPDEPS Gui Qml Quick QuickControls2 Widgets)

if (BUILD_TESTING)
	enable_testing()
	add_definitions(-DQS_TEST)
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

if (SERVICE_STATUS_NOTIFIER OR SERVICE_MPRIS)
	set(DBUS ON)
endif()

if (DBUS)
	list(APPEND QT_DEPS Qt6::DBus)
	list(APPEND QT_FPDEPS DBus)
endif()

find_package(Qt6 REQUIRED COMPONENTS ${QT_FPDEPS})

qt_standard_project_setup(REQUIRES 6.6)
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml_modules)

# pch breaks clang-tidy..... somehow
if (NOT NO_PCH)
	file(GENERATE
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pchstub.cpp
		CONTENT "// intentionally empty"
	)

	add_library(qt-pch ${CMAKE_CURRENT_BINARY_DIR}/pchstub.cpp)
	target_link_libraries(qt-pch PRIVATE ${QT_DEPS})
	target_precompile_headers(qt-pch PUBLIC
		<memory>
		<qobject.h>
		<qqmlengine.h>
		<qlist.h>
		<qcolor.h>
		<qquickitem.h>
		<qevent.h>
	)
endif()

function (qs_pch target)
	if (NOT NO_PCH)
		target_precompile_headers(${target} REUSE_FROM qt-pch)
		target_link_libraries(${target} PRIVATE ${QT_DEPS}) # required for gcc to accept the pch on plugin targets
  endif()
endfunction()

add_subdirectory(src)

if (USE_JEMALLOC)
	find_package(PkgConfig REQUIRED)
	# IMPORTED_TARGET not working for some reason
	pkg_check_modules(JEMALLOC REQUIRED jemalloc)
	target_link_libraries(quickshell PRIVATE ${JEMALLOC_LIBRARIES})
endif()

install(
	DIRECTORY ${CMAKE_BINARY_DIR}/qml_modules/
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt-6/qml
	FILES_MATCHING PATTERN "*"
)

install(CODE "
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E create_symlink \
		${CMAKE_INSTALL_FULL_BINDIR}/quickshell ${CMAKE_INSTALL_FULL_BINDIR}/qs
	)
")
