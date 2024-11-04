qt_add_library(quickshell-wayland-toplevel-management STATIC
	manager.cpp
	handle.cpp
	qml.cpp
)

qt_add_qml_module(quickshell-wayland-toplevel-management
	URI Quickshell.Wayland._ToplevelManagement
	VERSION 0.1
	DEPENDENCIES QtQml Quickshell Quickshell.Wayland
)

install_qml_module(quickshell-wayland-toplevel-management)

wl_proto(quickshell-wayland-toplevel-management
	wlr-foreign-toplevel-management-unstable-v1
	"${CMAKE_CURRENT_SOURCE_DIR}/wlr-foreign-toplevel-management-unstable-v1.xml"
)

target_link_libraries(quickshell-wayland-toplevel-management PRIVATE ${QT_DEPS} wayland-client)

qs_pch(quickshell-wayland-toplevel-management)
qs_pch(quickshell-wayland-toplevel-managementplugin)

target_link_libraries(quickshell PRIVATE quickshell-wayland-toplevel-managementplugin)
