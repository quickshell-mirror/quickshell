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
	datastream.cpp
)

qt_add_qml_module(quickshell URI Quickshell)

if (SOCKETS)
	target_sources(quickshell PRIVATE socket.cpp)
endif()

target_link_libraries(quickshell PRIVATE ${QT_DEPS})

if (TESTS)
	add_subdirectory(test)
endif()
