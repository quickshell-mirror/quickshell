qt_add_library(quickshell-io STATIC
	datastream.cpp
	process.cpp
	fileview.cpp
	ipccomm.cpp
	ipc.cpp
	ipchandler.cpp
)

add_library(quickshell-io-init OBJECT init.cpp)

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

install_qml_module(quickshell-io)

target_link_libraries(quickshell-io PRIVATE Qt::Quick)
target_link_libraries(quickshell-io-init PRIVATE Qt::Qml)

target_link_libraries(quickshell PRIVATE quickshell-ioplugin quickshell-io-init)

qs_module_pch(quickshell-io)

if (BUILD_TESTING)
	add_subdirectory(test)
endif()
