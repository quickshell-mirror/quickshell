qt_add_executable(quickshell main.cpp)

install(TARGETS quickshell RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(core)
add_subdirectory(io)

if (WAYLAND)
	add_subdirectory(wayland)
endif ()
