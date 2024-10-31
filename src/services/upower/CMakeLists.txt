set_source_files_properties(org.freedesktop.UPower.xml PROPERTIES
	CLASSNAME DBusUPowerService
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(DBUS_INTERFACES
	org.freedesktop.UPower.xml
	dbus_service
)

set_source_files_properties(org.freedesktop.UPower.Device.xml PROPERTIES
	CLASSNAME DBusUPowerDevice
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(DBUS_INTERFACES
	org.freedesktop.UPower.Device.xml
	dbus_device
)

qt_add_library(quickshell-service-upower STATIC
	core.cpp
	device.cpp
	${DBUS_INTERFACES}
)

# dbus headers
target_include_directories(quickshell-service-upower PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

qt_add_qml_module(quickshell-service-upower
	URI Quickshell.Services.UPower
  VERSION 0.1
	DEPENDENCIES QtQml Quickshell
)

target_link_libraries(quickshell-service-upower PRIVATE ${QT_DEPS} quickshell-dbus)
target_link_libraries(quickshell PRIVATE quickshell-service-upowerplugin)

qs_pch(quickshell-service-upower)
qs_pch(quickshell-service-upowerplugin)
