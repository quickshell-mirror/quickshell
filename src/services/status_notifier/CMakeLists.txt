qt_add_dbus_adaptor(DBUS_INTERFACES
	org.kde.StatusNotifierWatcher.xml
	watcher.hpp
	qs::service::sni::StatusNotifierWatcher
	dbus_watcher
	StatusNotifierWatcherAdaptor
)

set_source_files_properties(org.kde.StatusNotifierItem.xml PROPERTIES
	CLASSNAME DBusStatusNotifierItem
	INCLUDE dbus_item_types.hpp
)

qt_add_dbus_interface(DBUS_INTERFACES
	org.kde.StatusNotifierItem.xml
	dbus_item
)

set_source_files_properties(org.kde.StatusNotifierWatcher.xml PROPERTIES
	CLASSNAME DBusStatusNotifierWatcher
)

qt_add_dbus_interface(DBUS_INTERFACES
	org.kde.StatusNotifierWatcher.xml
	dbus_watcher_interface
)

qt_add_library(quickshell-service-statusnotifier STATIC
	qml.cpp

	watcher.cpp
	host.cpp
	item.cpp
	dbus_item_types.cpp
	${DBUS_INTERFACES}
)

# dbus headers
target_include_directories(quickshell-service-statusnotifier PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

qt_add_qml_module(quickshell-service-statusnotifier
  URI Quickshell.Services.SystemTray
  VERSION 0.1
)

target_link_libraries(quickshell-service-statusnotifier PRIVATE ${QT_DEPS} quickshell-dbus)
target_link_libraries(quickshell PRIVATE quickshell-service-statusnotifierplugin)

qs_pch(quickshell-service-statusnotifier)
qs_pch(quickshell-service-statusnotifierplugin)
