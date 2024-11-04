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
	DEPENDENCIES QtQml Quickshell
)

install_qml_module(quickshell-service-mpris)

target_link_libraries(quickshell-service-mpris PRIVATE ${QT_DEPS} quickshell-dbus)
target_link_libraries(quickshell PRIVATE quickshell-service-mprisplugin)

qs_pch(quickshell-service-mpris)
qs_pch(quickshell-service-mprisplugin)
