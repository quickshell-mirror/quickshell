find_package(PkgConfig REQUIRED)
pkg_check_modules(pipewire REQUIRED IMPORTED_TARGET libpipewire-0.3)

qt_add_library(quickshell-service-pipewire STATIC
	qml.cpp
	core.cpp
	connection.cpp
	registry.cpp
	node.cpp
	metadata.cpp
	link.cpp
	device.cpp
	defaults.cpp
)

qt_add_qml_module(quickshell-service-pipewire
	URI Quickshell.Services.Pipewire
	VERSION 0.1
	DEPENDENCIES QtQml Quickshell
)

install_qml_module(quickshell-service-pipewire)

target_link_libraries(quickshell-service-pipewire PRIVATE ${QT_DEPS} PkgConfig::pipewire)

qs_pch(quickshell-service-pipewire)
qs_pch(quickshell-service-pipewireplugin)

target_link_libraries(quickshell PRIVATE quickshell-service-pipewireplugin)
