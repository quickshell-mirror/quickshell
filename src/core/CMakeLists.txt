qt_add_library(quickshell-core STATIC
	main.cpp
	plugin.cpp
	shell.cpp
	variants.cpp
	rootwrapper.cpp
	proxywindow.cpp
	reload.cpp
	rootwrapper.cpp
	qmlglobal.cpp
	qmlscreen.cpp
	watcher.cpp
	region.cpp
	persistentprops.cpp
	windowinterface.cpp
	floatingwindow.cpp
	panelinterface.cpp
	popupwindow.cpp
	singleton.cpp
	generation.cpp
)

set_source_files_properties(main.cpp PROPERTIES COMPILE_DEFINITIONS GIT_REVISION="${GIT_REVISION}")
qt_add_qml_module(quickshell-core URI Quickshell VERSION 0.1)

target_link_libraries(quickshell-core PRIVATE ${QT_DEPS})
qs_pch(quickshell-core)

target_link_libraries(quickshell PRIVATE quickshell-coreplugin)

if (BUILD_TESTING)
	add_subdirectory(test)
endif()
