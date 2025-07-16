# NetworkManager DBus
set_source_files_properties(org.freedesktop.NetworkManager.xml PROPERTIES
	CLASSNAME DBusNetworkManagerProxy
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(NM_DBUS_INTERFACES
	org.freedesktop.NetworkManager.xml
	dbus_nm_backend
)

set_source_files_properties(org.freedesktop.NetworkManager.Device.xml PROPERTIES
	CLASSNAME DBusNMDeviceProxy
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(NM_DBUS_INTERFACES
	org.freedesktop.NetworkManager.Device.xml
	dbus_nm_device
)

set_source_files_properties(org.freedesktop.NetworkManager.Device.Wireless.xml PROPERTIES
	CLASSNAME DBusNMWirelessProxy
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(NM_DBUS_INTERFACES
	org.freedesktop.NetworkManager.Device.Wireless.xml
	dbus_nm_wireless
)

set_source_files_properties(org.freedesktop.NetworkManager.AccessPoint.xml PROPERTIES
	CLASSNAME DBusNMAccessPointProxy
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(NM_DBUS_INTERFACES
	org.freedesktop.NetworkManager.AccessPoint.xml
	dbus_nm_accesspoint
)

qt_add_library(quickshell-network STATIC
	api.cpp
	nm_backend.cpp
	nm_adapters.cpp
	${NM_DBUS_INTERFACES}
)

# dbus headers
target_include_directories(quickshell-network PRIVATE 
	${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_qml_module(quickshell-network
	URI Quickshell.Network
	VERSION 0.1
	DEPENDENCIES QtQml
)

qs_add_module_deps_light(quickshell-network Quickshell)
install_qml_module(quickshell-network)

target_link_libraries(quickshell-network PRIVATE Qt::Qml Qt::DBus)
qs_add_link_dependencies(quickshell-network quickshell-dbus)
target_link_libraries(quickshell PRIVATE quickshell-networkplugin)

qs_module_pch(quickshell-network SET dbus)
