find_package(XCB REQUIRED COMPONENTS XCB)

qt_add_library(quickshell-x11 STATIC
	util.cpp
	panel_window.cpp
)

qt_add_qml_module(quickshell-x11
	URI Quickshell.X11
	VERSION 0.1
	DEPENDENCIES QtQuick
)

if(I3)
	add_subdirectory(i3)
endif()

install_qml_module(quickshell-x11)

add_library(quickshell-x11-init OBJECT init.cpp)

target_link_libraries(quickshell-x11 PRIVATE Qt::Quick ${XCB_LIBRARIES})
target_link_libraries(quickshell-x11-init PRIVATE Qt::Quick Qt::Qml ${XCB_LIBRARIES})

qs_module_pch(quickshell-x11 SET large)

target_link_libraries(quickshell PRIVATE quickshell-x11plugin quickshell-x11-init)
