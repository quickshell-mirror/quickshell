qt_add_library(quickshell-i3 STATIC)

target_link_libraries(quickshell-i3 PRIVATE ${QT_DEPS})

set(I3_MODULES)

if (I3_IPC)
	add_subdirectory(ipc)
	list(APPEND I3_MODULES Quickshell.I3._Ipc)
endif()

qt_add_qml_module(quickshell-i3
	URI Quickshell.I3
	VERSION 0.1
	IMPORTS ${I3_MODULES}
)

install_qml_module(quickshell-i3)

qs_pch(quickshell-i3)
qs_pch(quickshell-i3plugin)

target_link_libraries(quickshell PRIVATE quickshell-i3plugin)
