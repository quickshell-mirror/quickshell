set_source_files_properties(org.freedesktop.NetworkManager.xml PROPERTIES
	CLASSNAME DBusNetworkManagerProxy
	NO_NAMESPACE TRUE
	INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/types.hpp
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

set_source_files_properties(org.freedesktop.NetworkManager.Settings.Connection.xml PROPERTIES
	CLASSNAME DBusNMConnectionSettingsProxy
	NO_NAMESPACE TRUE
	INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/types.hpp
)

qt_add_dbus_interface(NM_DBUS_INTERFACES
	org.freedesktop.NetworkManager.Settings.Connection.xml
	dbus_nm_connection_settings
)

set_source_files_properties(org.freedesktop.NetworkManager.Connection.Active.xml PROPERTIES
	CLASSNAME DBusNMActiveConnectionProxy
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(NM_DBUS_INTERFACES
	org.freedesktop.NetworkManager.Connection.Active.xml
	dbus_nm_active_connection
)

qt_add_library(quickshell-network-nm STATIC
	backend.cpp
	device.cpp
	connection.cpp
	accesspoint.cpp
	wireless.cpp
	utils.cpp
	enums.hpp
	${NM_DBUS_INTERFACES}
)

target_include_directories(quickshell-network-nm PUBLIC 
	${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(quickshell-network-nm PRIVATE Qt::Qml Qt::DBus)
qs_add_link_dependencies(quickshell-network-nm quickshell-dbus)
