set_source_files_properties(org.freedesktop.DBus.Properties.xml PROPERTIES
	CLASSNAME DBusPropertiesInterface
)

qt_add_dbus_interface(DBUS_INTERFACES
	org.freedesktop.DBus.Properties.xml
	dbus_properties
)

qt_add_library(quickshell-dbus STATIC
	properties.cpp
	${DBUS_INTERFACES}
)

# dbus headers
target_include_directories(quickshell-dbus PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(quickshell-dbus PRIVATE ${QT_DEPS})

qs_pch(quickshell-dbus)
#qs_pch(quickshell-dbusplugin)
