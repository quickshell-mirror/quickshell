qt_add_executable(quickshell
	main.cpp
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
	shellwindow.cpp
)

qt_add_qml_module(quickshell URI QuickShell)

target_link_libraries(quickshell PRIVATE ${QT_DEPS} quickshell-waylandplugin)
