qt_add_executable(quickshell main.cpp)

install(TARGETS quickshell RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(build)
add_subdirectory(launch)
add_subdirectory(core)
add_subdirectory(debug)
add_subdirectory(ipc)
add_subdirectory(window)
add_subdirectory(io)
add_subdirectory(widgets)
add_subdirectory(ui)

if (CRASH_REPORTER)
	add_subdirectory(crash)
endif()

if (DBUS)
	add_subdirectory(dbus)
endif()

if (WAYLAND)
	add_subdirectory(wayland)
endif()

if (X11)
	add_subdirectory(x11)
endif()

add_subdirectory(services)

if (BLUETOOTH)
	add_subdirectory(bluetooth)
endif()

if (NETWORK)
	add_subdirectory(network)
endif()
