qt_add_executable(quickshell main.cpp)

target_link_libraries(quickshell PRIVATE ${QT_DEPS} quickshell-build)

install(TARGETS quickshell RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(build)
add_subdirectory(core)
add_subdirectory(ipc)
add_subdirectory(window)
add_subdirectory(io)
add_subdirectory(widgets)

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
