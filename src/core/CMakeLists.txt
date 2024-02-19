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

# qml type registration requires this
target_include_directories(quickshell PRIVATE src/core)

target_link_libraries(quickshell PRIVATE Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2)

if (LAYERSHELL)
	find_package(LayerShellQt REQUIRED)

	target_link_libraries(quickshell PRIVATE LayerShellQtInterface)
	target_compile_definitions(quickshell PRIVATE CONF_LAYERSHELL)

	target_sources(quickshell PRIVATE layershell.cpp)
endif()
