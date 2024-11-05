qt_add_library(quickshell-ipc STATIC
	ipc.cpp
)

qs_pch(quickshell-ipc)

target_link_libraries(quickshell-ipc PRIVATE Qt::Quick Qt::Network)

target_link_libraries(quickshell PRIVATE quickshell-ipc)
