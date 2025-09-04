set_source_files_properties(nm/org.freedesktop.NetworkManager.xml PROPERTIES
	CLASSNAME DBusNetworkManagerProxy
	NO_NAMESPACE TRUE
	INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/nm/dbus_types.hpp
)

qt_add_dbus_interface(NM_DBUS_INTERFACES
	nm/org.freedesktop.NetworkManager.xml
	nm/dbus_nm_backend
)

set_source_files_properties(nm/org.freedesktop.NetworkManager.Device.xml PROPERTIES
	CLASSNAME DBusNMDeviceProxy
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(NM_DBUS_INTERFACES
	nm/org.freedesktop.NetworkManager.Device.xml
	nm/dbus_nm_device
)

set_source_files_properties(nm/org.freedesktop.NetworkManager.Device.Wireless.xml PROPERTIES
	CLASSNAME DBusNMWirelessProxy
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(NM_DBUS_INTERFACES
	nm/org.freedesktop.NetworkManager.Device.Wireless.xml
	nm/dbus_nm_wireless
)

set_source_files_properties(nm/org.freedesktop.NetworkManager.AccessPoint.xml PROPERTIES
	CLASSNAME DBusNMAccessPointProxy
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(NM_DBUS_INTERFACES
	nm/org.freedesktop.NetworkManager.AccessPoint.xml
	nm/dbus_nm_accesspoint
)

set_source_files_properties(nm/org.freedesktop.NetworkManager.Settings.Connection.xml PROPERTIES
	CLASSNAME DBusNMConnectionSettingsProxy
	NO_NAMESPACE TRUE
	INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/nm/dbus_types.hpp
)

qt_add_dbus_interface(NM_DBUS_INTERFACES
	nm/org.freedesktop.NetworkManager.Settings.Connection.xml
	nm/dbus_nm_connection_settings
)

set_source_files_properties(nm/org.freedesktop.NetworkManager.Connection.Active.xml PROPERTIES
	CLASSNAME DBusNMActiveConnectionProxy
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(NM_DBUS_INTERFACES
	nm/org.freedesktop.NetworkManager.Connection.Active.xml
	nm/dbus_nm_active_connection
)

qt_add_library(quickshell-network STATIC
	network.cpp
	device.cpp
	wifi.cpp
	nm/backend.cpp
	nm/device.cpp
	nm/connection.cpp
	nm/accesspoint.cpp
	nm/wireless.cpp
	nm/utils.cpp
	nm/enums.hpp
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
