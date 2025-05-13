find_package(PkgConfig REQUIRED)
# Not using libnm
#pkg_check_modules(networkmanager REQUIRED IMPORTED_TARGET libnm)

set_source_files_properties(org.freedesktop.NetworkManager.xml PROPERTIES
	CLASSNAME DBusNetworkManager
	NO_NAMESPACE TRUE
)
qt_add_dbus_interface(DBUS_INTERFACES
	org.freedesktop.NetworkManager.xml
	dbus_service
)

qt_add_library(quickshell-service-networkmanager STATIC
	core.cpp
	${DBUS_INTERFACES}
)

# dbus headers
target_include_directories(quickshell-service-networkmanager PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

qt_add_qml_module(quickshell-service-networkmanager
	URI Quickshell.Services.Networkmanager
	VERSION 0.1
	DEPENDENCIES QtQml
)

# Not sure what this is doing
qs_add_module_deps_light(quickshell-service-networkmanager Quickshell)

install_qml_module(quickshell-service-networkmanager)

target_link_libraries(quickshell-service-networkmanager PRIVATE
	Qt::Qml
	Qt::Quick # pch
)

qs_module_pch(quickshell-service-networkmanager)
