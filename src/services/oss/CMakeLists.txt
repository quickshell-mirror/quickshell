qt_add_library(quickshell-service-oss STATIC
	oss.cpp
)

qt_add_qml_module(quickshell-service-oss
	URI Quickshell.Services.OSS
	VERSION 0.1
	DEPENDENCIES QtQml
)

install_qml_module(quickshell-service-oss)

target_link_libraries(quickshell PRIVATE quickshell-service-ossplugin Qt6::Core)
