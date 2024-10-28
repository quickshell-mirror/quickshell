qt_add_library(quickshell-ipc STATIC
	ipc.cpp
)

target_link_libraries(quickshell-ipc PRIVATE ${QT_DEPS})

target_link_libraries(quickshell PRIVATE quickshell-ipc)
