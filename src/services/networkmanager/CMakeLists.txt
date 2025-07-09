set_source_files_properties(org.freedesktop.NetworkManager.xml PROPERTIES
	CLASSNAME DBusNetworkManagerService
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(DBUS_INTERFACES
	org.freedesktop.NetworkManager.xml
	dbus_service
)

set_source_files_properties(org.freedesktop.NetworkManager.Device.xml PROPERTIES
	CLASSNAME DBusNMDevice
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(DBUS_INTERFACES
	org.freedesktop.NetworkManager.Device.xml
	dbus_device
)

qt_add_library(quickshell-service-networkmanager STATIC
	core.cpp
	device.cpp
	${DBUS_INTERFACES}
)

# dbus headers
target_include_directories(quickshell-service-networkmanager PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

qt_add_qml_module(quickshell-service-networkmanager
	URI Quickshell.Services.NetworkManager
	VERSION 0.1
	DEPENDENCIES QtQml
)

qs_add_module_deps_light(quickshell-service-networkmanager Quickshell)
install_qml_module(quickshell-service-networkmanager)

target_link_libraries(quickshell-service-networkmanager PRIVATE Qt::Qml Qt::DBus)
qs_add_link_dependencies(quickshell-service-networkmanager quickshell-dbus)
target_link_libraries(quickshell PRIVATE quickshell-service-networkmanagerplugin)

qs_module_pch(quickshell-service-networkmanager SET dbus)
