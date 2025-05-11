find_package(PkgConfig REQUIRED)
pkg_check_modules(networkmanager REQUIRED IMPORTED_TARGET libnm)

qt_add_library(quickshell-service-networkmanager STATIC
	core.cpp
)

qt_add_qml_module(quickshell-service-networkmanager
	URI Quickshell.Services.Networkmanager
	VERSION 0.1
	DEPENDENCIES QtQml
)

qs_add_module_deps_light(quickshell-service-networkmanager Quickshell)

install_qml_module(quickshell-service-networkmanager)

target_link_libraries(quickshell-service-networkmanager PRIVATE
	Qt::Qml PkgConfig::networkmanager
	Qt::Quick # pch
)

qs_module_pch(quickshell-service-networkmanager)
