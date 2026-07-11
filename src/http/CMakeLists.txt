qt_add_library(quickshell-http STATIC
	client.cpp
	response.cpp
)

qt_add_qml_module(quickshell-http
	URI Quickshell.Http
	VERSION 0.1
	DEPENDENCIES QtQml
)

install_qml_module(quickshell-http)

target_link_libraries(quickshell-http PRIVATE
	Qt::Qml
	Qt::Network
)

qs_module_pch(quickshell-http)

target_link_libraries(quickshell PRIVATE quickshell-httpplugin)
