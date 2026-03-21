add_subdirectory(nm)

qt_add_library(quickshell-network STATIC
	network.cpp
	device.cpp
	wifi.cpp
)

target_include_directories(quickshell-network PRIVATE 
	${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_qml_module(quickshell-network
	URI Quickshell.Networking
	VERSION 0.1
	DEPENDENCIES QtQml
)

qs_add_module_deps_light(quickshell-network Quickshell)
install_qml_module(quickshell-network)
target_link_libraries(quickshell-network PRIVATE quickshell-network-nm Qt::Qml Qt::DBus)
qs_add_link_dependencies(quickshell-network quickshell-dbus)
target_link_libraries(quickshell PRIVATE quickshell-networkplugin)
qs_module_pch(quickshell-network SET dbus)
