set_source_files_properties(org.bluez.Adapter.xml PROPERTIES
	CLASSNAME DBusBluezAdapterInterface
)

set_source_files_properties(org.bluez.Device.xml PROPERTIES
	CLASSNAME DBusBluezDeviceInterface
)

qt_add_dbus_interface(DBUS_INTERFACES
	org.bluez.Adapter.xml
	dbus_adapter
)

qt_add_dbus_interface(DBUS_INTERFACES
	org.bluez.Device.xml
	dbus_device
)

qt_add_library(quickshell-bluetooth STATIC
	adapter.cpp
	bluez.cpp
	device.cpp
	${DBUS_INTERFACES}
)

qt_add_qml_module(quickshell-bluetooth
	URI Quickshell.Bluetooth
	VERSION 0.1
	DEPENDENCIES QtQml
)

install_qml_module(quickshell-bluetooth)

# dbus headers
target_include_directories(quickshell-bluetooth PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(quickshell-bluetooth PRIVATE Qt::Qml Qt::DBus)
qs_add_link_dependencies(quickshell-bluetooth quickshell-dbus)

qs_module_pch(quickshell-bluetooth SET dbus)

target_link_libraries(quickshell PRIVATE quickshell-bluetoothplugin)
