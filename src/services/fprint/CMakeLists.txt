set_source_files_properties(net.reactivated.Fprint.Manager.xml PROPERTIES
	CLASSNAME DBusFprintService
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(DBUS_INTERFACES
	net.reactivated.Fprint.Manager.xml
	dbus_service
)

set_source_files_properties(net.reactivated.Fprint.Device.xml PROPERTIES
	CLASSNAME DBusFprintDevice
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(DBUS_INTERFACES
	net.reactivated.Fprint.Device.xml
	dbus_device
)

qt_add_library(quickshell-service-fprint STATIC
	core.cpp
	device.cpp
	${DBUS_INTERFACES}
)

# dbus headers
target_include_directories(quickshell-service-fprint PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

qt_add_qml_module(quickshell-service-fprint
	URI Quickshell.Services.Fprint
  VERSION 0.1
	DEPENDENCIES QtQml
)

qs_add_module_deps_light(quickshell-service-fprint quickshell-core)

install_qml_module(quickshell-service-fprint)

target_link_libraries(quickshell-service-fprint PRIVATE Qt::Qml Qt::DBus)
qs_add_link_dependencies(quickshell-service-fprint quickshell-dbus)
target_link_libraries(quickshell PRIVATE quickshell-service-fprintplugin)

qs_module_pch(quickshell-service-fprint SET dbus)
