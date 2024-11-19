set_source_files_properties(com.canonical.dbusmenu.xml PROPERTIES
	CLASSNAME DBusMenuInterface
	INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/dbus_menu_types.hpp
)

qt_add_dbus_interface(DBUS_INTERFACES
	com.canonical.dbusmenu.xml
	dbus_menu
)

qt_add_library(quickshell-dbusmenu STATIC
	dbus_menu_types.cpp
	dbusmenu.cpp
	${DBUS_INTERFACES}
)

qt_add_qml_module(quickshell-dbusmenu
	URI Quickshell.DBusMenu
	VERSION 0.1
	DEPENDENCIES QtQml
)

qs_add_module_deps_light(quickshell-dbusmenu Quickshell)

install_qml_module(quickshell-dbusmenu)

# dbus headers
target_include_directories(quickshell-dbusmenu PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(quickshell-dbusmenu PRIVATE Qt::Quick Qt::DBus)
qs_add_link_dependencies(quickshell-dbusmenu quickshell-dbus)

qs_module_pch(quickshell-dbusmenu SET dbus)

target_link_libraries(quickshell PRIVATE quickshell-dbusmenuplugin)
