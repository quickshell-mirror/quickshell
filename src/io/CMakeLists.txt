qt_add_library(quickshell-io STATIC
	datastream.cpp
)

if (SOCKETS)
	target_sources(quickshell-io PRIVATE socket.cpp)
endif()

qt_add_qml_module(quickshell-io URI Quickshell.Io)
target_link_libraries(quickshell-io PRIVATE ${QT_DEPS})

target_link_libraries(quickshell PRIVATE quickshell-ioplugin)

if (TESTS)
	add_subdirectory(test)
endif()
