cmake_minimum_required(VERSION 3.20)
project(quickshell VERSION "0.1.0" LANGUAGES CXX C)

set(QT_MIN_VERSION "6.6.0")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTING "Build tests" OFF)
option(ASAN "Enable ASAN" OFF) # note: better output with gcc than clang
option(FRAME_POINTERS "Always keep frame pointers" ${ASAN})

option(CRASH_REPORTER "Enable the crash reporter" ON)
option(USE_JEMALLOC "Use jemalloc over the system malloc implementation" ON)
option(SOCKETS "Enable unix socket support" ON)
option(WAYLAND "Enable wayland support" ON)
option(WAYLAND_WLR_LAYERSHELL "Support the zwlr_layer_shell_v1 wayland protocol" ON)
option(WAYLAND_SESSION_LOCK "Support the ext_session_lock_v1 wayland protocol" ON)
option(WAYLAND_TOPLEVEL_MANAGEMENT "Support the zwlr_foreign_toplevel_management_v1 wayland protocol" ON)
option(X11 "Enable X11 support" ON)
option(HYPRLAND "Support hyprland specific features" ON)
option(HYPRLAND_IPC "Hyprland IPC" ON)
option(HYPRLAND_GLOBAL_SHORTCUTS "Hyprland Global Shortcuts" ON)
option(HYPRLAND_FOCUS_GRAB "Hyprland Focus Grabbing" ON)
option(SERVICE_STATUS_NOTIFIER "StatusNotifierItem service" ON)
option(SERVICE_PIPEWIRE "PipeWire service" ON)
option(SERVICE_MPRIS "Mpris service" ON)
option(SERVICE_PAM "Pam service" ON)
option(SERVICE_GREETD "Greet service" ON)
option(SERVICE_UPOWER "UPower service" ON)
option(SERVICE_NOTIFICATIONS "Notification server" ON)

message(STATUS "Quickshell configuration")
message(STATUS "  Crash reporter: ${CRASH_REPORTER}")
message(STATUS "  Jemalloc: ${USE_JEMALLOC}")
message(STATUS "  Build tests: ${BUILD_TESTING}")
message(STATUS "  Sockets: ${SOCKETS}")
message(STATUS "  Wayland: ${WAYLAND}")
if (WAYLAND)
	message(STATUS "    Wlroots Layershell: ${WAYLAND_WLR_LAYERSHELL}")
	message(STATUS "    Session Lock: ${WAYLAND_SESSION_LOCK}")
	message(STATUS "    Toplevel Management: ${WAYLAND_TOPLEVEL_MANAGEMENT}")
endif ()
message(STATUS "  X11: ${X11}")
message(STATUS "  Services")
message(STATUS "    StatusNotifier: ${SERVICE_STATUS_NOTIFIER}")
message(STATUS "    PipeWire: ${SERVICE_PIPEWIRE}")
message(STATUS "    Mpris: ${SERVICE_MPRIS}")
message(STATUS "    Pam: ${SERVICE_PAM}")
message(STATUS "    Greetd: ${SERVICE_GREETD}")
message(STATUS "    UPower: ${SERVICE_UPOWER}")
message(STATUS "    Notifications: ${SERVICE_NOTIFICATIONS}")
message(STATUS "  Hyprland: ${HYPRLAND}")
if (HYPRLAND)
	message(STATUS "    IPC: ${HYPRLAND_IPC}")
	message(STATUS "    Focus Grabbing: ${HYPRLAND_FOCUS_GRAB}")
	message(STATUS "    Global Shortcuts: ${HYPRLAND_GLOBAL_SHORTCUTS}")
endif()

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
