set_source_files_properties(org.mpris.MediaPlayer2.Player.xml PROPERTIES
	CLASSNAME DBusMprisPlayer
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(DBUS_INTERFACES
	org.mpris.MediaPlayer2.Player.xml
	dbus_player
)

set_source_files_properties(org.mpris.MediaPlayer2.xml PROPERTIES
	CLASSNAME DBusMprisPlayerApp
	NO_NAMESPACE TRUE
)

qt_add_dbus_interface(DBUS_INTERFACES
	org.mpris.MediaPlayer2.xml
	dbus_player_app
)

qt_add_library(quickshell-service-mpris STATIC
	player.cpp
	watcher.cpp
	${DBUS_INTERFACES}
)

# dbus headers
target_include_directories(quickshell-service-mpris PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

qt_add_qml_module(quickshell-service-mpris
	URI Quickshell.Services.Mpris
  VERSION 0.1
	DEPENDENCIES QtQml
)

qs_add_module_deps_light(quickshell-service-mpris Quickshell)

install_qml_module(quickshell-service-mpris)

target_link_libraries(quickshell-service-mpris PRIVATE Qt::Qml Qt::DBus)
qs_add_link_dependencies(quickshell-service-mpris quickshell-dbus)

qs_module_pch(quickshell-service-mpris SET dbus)

target_link_libraries(quickshell PRIVATE quickshell-service-mprisplugin)
