set_source_files_properties(org.freedesktop.NetworkManager.xml PROPERTIES
	CLASSNAME DBusNetworkManager
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(NM_DBUS_INTERFACES
	org.freedesktop.NetworkManager.xml
	dbus_nm_backend
)

set_source_files_properties(org.freedesktop.NetworkManager.Device.xml PROPERTIES
	CLASSNAME DBusNMDevice
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(NM_DBUS_INTERFACES
	org.freedesktop.NetworkManager.Device.xml
	dbus_nm_device
)

set_source_files_properties(org.freedesktop.NetworkManager.Device.Wireless.xml PROPERTIES
	CLASSNAME DBusNMWireless
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(NM_DBUS_INTERFACES
	org.freedesktop.NetworkManager.Device.Wireless.xml
	dbus_nm_wireless
)

set(NM_DBUS_INTERFACES ${NM_DBUS_INTERFACES} PARENT_SCOPE)
set(NM_DBUS_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)
