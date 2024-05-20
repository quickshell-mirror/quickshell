find_package(XCB REQUIRED COMPONENTS XCB)

qt_add_library(quickshell-x11 STATIC
	util.cpp
	panel_window.cpp
)

qt_add_qml_module(quickshell-x11
	URI Quickshell.X11
	VERSION 0.1
)

add_library(quickshell-x11-init OBJECT init.cpp)

target_link_libraries(quickshell-x11 PRIVATE ${QT_DEPS} ${XCB_LIBRARIES})
target_link_libraries(quickshell-x11-init PRIVATE ${QT_DEPS} ${XCB_LIBRARIES})

qs_pch(quickshell-x11)
qs_pch(quickshell-x11plugin)
qs_pch(quickshell-x11-init)

target_link_libraries(quickshell PRIVATE quickshell-x11plugin quickshell-x11-init)
