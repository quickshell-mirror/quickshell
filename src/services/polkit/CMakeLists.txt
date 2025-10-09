find_package(PkgConfig REQUIRED)
pkg_check_modules(glib REQUIRED IMPORTED_TARGET glib-2.0>=2.36)
pkg_check_modules(gobject REQUIRED IMPORTED_TARGET gobject-2.0)
pkg_check_modules(polkit_agent REQUIRED IMPORTED_TARGET polkit-agent-1)
pkg_check_modules(polkit REQUIRED IMPORTED_TARGET polkit-gobject-1)

qt_add_library(quickshell-service-polkit STATIC
	agentimpl.cpp
	flow.cpp
	identity.cpp
	listener.cpp
	session.cpp
	qml.cpp
)

qt_add_qml_module(quickshell-service-polkit
	URI Quickshell.Services.Polkit
	VERSION 0.1
	DEPENDENCIES QtQml
)

install_qml_module(quickshell-service-polkit)

target_link_libraries(quickshell-service-polkit PRIVATE
	Qt::Qml
	Qt::Quick
	PkgConfig::glib
	PkgConfig::gobject
	PkgConfig::polkit_agent
	PkgConfig::polkit
)

qs_module_pch(quickshell-service-polkit)

target_link_libraries(quickshell PRIVATE quickshell-service-polkitplugin)
