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
	powerprofiles.cpp
	${DBUS_INTERFACES}
)

# dbus headers
target_include_directories(quickshell-service-upower PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

qt_add_qml_module(quickshell-service-upower
	URI Quickshell.Services.UPower
  VERSION 0.1
	DEPENDENCIES QtQml
)

qs_add_module_deps_light(quickshell-service-upower Quickshell)

install_qml_module(quickshell-service-upower)

target_link_libraries(quickshell-service-upower PRIVATE Qt::Qml Qt::DBus)
qs_add_link_dependencies(quickshell-service-upower quickshell-dbus)
target_link_libraries(quickshell PRIVATE quickshell-service-upowerplugin)

qs_module_pch(quickshell-service-upower SET dbus)
