qt_add_library(quickshell-io STATIC
	datastream.cpp
	processcore.cpp
	process.cpp
	fileview.cpp
	jsonadapter.cpp
	ipccomm.cpp
	ipc.cpp
	ipchandler.cpp
)

if (SOCKETS)
	target_sources(quickshell-io PRIVATE socket.cpp)
endif()

qt_add_qml_module(quickshell-io
	URI Quickshell.Io
	VERSION 0.1
	DEPENDENCIES QtQml
	QML_FILES
		FileView.qml
)

qs_add_module_deps_light(quickshell-io Quickshell)
install_qml_module(quickshell-io)

target_link_libraries(quickshell-io PRIVATE Qt::Quick quickshell-ipc)
target_link_libraries(quickshell PRIVATE quickshell-ioplugin)

qs_module_pch(quickshell-io)

if (BUILD_TESTING)
	add_subdirectory(test)
endif()
