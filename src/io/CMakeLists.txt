qt_add_library(quickshell-io STATIC
	datastream.cpp
	process.cpp
)

add_library(quickshell-io-init OBJECT init.cpp)

if (SOCKETS)
	target_sources(quickshell-io PRIVATE socket.cpp)
endif()

qt_add_qml_module(quickshell-io URI Quickshell.Io VERSION 0.1)

target_link_libraries(quickshell-io PRIVATE ${QT_DEPS})
target_link_libraries(quickshell-io-init PRIVATE ${QT_DEPS})

target_link_libraries(quickshell PRIVATE quickshell-ioplugin quickshell-io-init)

if (BUILD_TESTING)
	add_subdirectory(test)
endif()
