qt_add_library(quickshell-niri STATIC)

target_link_libraries(quickshell-niri PRIVATE ${QT_DEPS})

set(NIRI_MODULES)

if (NIRI_IPC)
	add_subdirectory(ipc)
	list(APPEND NIRI_MODULES Quickshell.Niri._Ipc)
endif()

qt_add_qml_module(quickshell-niri
	URI Quickshell.Niri
	VERSION 0.1
	IMPORTS ${NIRI_MODULES}
)

qs_add_module_deps_light(quickshell-niri Quickshell)
install_qml_module(quickshell-niri)

# intentionally no pch as the module is empty

target_link_libraries(quickshell PRIVATE quickshell-niriplugin)
