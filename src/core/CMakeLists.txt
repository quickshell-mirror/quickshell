qt_add_executable(quickshell
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
)

set_source_files_properties(main.cpp PROPERTIES COMPILE_DEFINITIONS GIT_REVISION="${GIT_REVISION}")
qt_add_qml_module(quickshell URI Quickshell VERSION 0.1)

target_link_libraries(quickshell PRIVATE ${QT_DEPS})
qs_pch(quickshell)

if (BUILD_TESTING)
	add_subdirectory(test)
endif()
