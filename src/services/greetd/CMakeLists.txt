qt_add_library(quickshell-service-greetd STATIC
	qml.cpp
	connection.cpp
)

qt_add_qml_module(quickshell-service-greetd
	URI Quickshell.Services.Greetd
	VERSION 0.1
)

target_link_libraries(quickshell-service-greetd PRIVATE ${QT_DEPS})

qs_pch(quickshell-service-greetd)
qs_pch(quickshell-service-greetdplugin)

target_link_libraries(quickshell PRIVATE quickshell-service-greetdplugin)
