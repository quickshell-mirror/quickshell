qt_add_library(quickshell-service-greetd STATIC
	qml.cpp
	connection.cpp
)

qt_add_qml_module(quickshell-service-greetd
	URI Quickshell.Services.Greetd
	VERSION 0.1
	DEPENDENCIES QtQml
)

install_qml_module(quickshell-service-greetd)

# can't be Qt::Qml because generation.hpp pulls in gui types
target_link_libraries(quickshell-service-greetd PRIVATE Qt::Quick)

qs_module_pch(quickshell-service-greetd)

target_link_libraries(quickshell PRIVATE quickshell-service-greetdplugin)
