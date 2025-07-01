set_source_files_properties(org.freedesktop.DBus.Properties.xml PROPERTIES
	CLASSNAME DBusPropertiesInterface
)

set_source_files_properties(org.freedesktop.DBus.ObjectManager.xml PROPERTIES
	CLASSNAME DBusObjectManagerInterface
	INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/dbus_objectmanager_types.hpp
)

qt_add_dbus_interface(DBUS_INTERFACES
	org.freedesktop.DBus.Properties.xml
	dbus_properties
)

qt_add_dbus_interface(DBUS_INTERFACES
	org.freedesktop.DBus.ObjectManager.xml
	dbus_objectmanager
)

qt_add_library(quickshell-dbus STATIC
	properties.cpp
	objectmanager.cpp
	bus.cpp
	${DBUS_INTERFACES}
)

# dbus headers
target_include_directories(quickshell-dbus PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(quickshell-dbus PRIVATE Qt::Core Qt::DBus)
# todo: link dbus to quickshell here instead of in modules that use it directly
# linker does not like this as is

qs_add_pchset(dbus
	DEPENDENCIES Qt::DBus
	HEADERS
		<QtDBus>
		<qdebug.h>
		<qdbusargument.h>
)

qs_pch(quickshell-dbus SET dbus)

add_subdirectory(dbusmenu)
