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

target_link_libraries(quickshell PRIVATE ${QT_DEPS} quickshell-wayland)

if (LAYERSHELL)
	find_package(LayerShellQt REQUIRED)

	target_link_libraries(quickshell PRIVATE LayerShellQtInterface)
	target_compile_definitions(quickshell PRIVATE CONF_LAYERSHELL)

	target_sources(quickshell PRIVATE layershell.cpp)
endif()
