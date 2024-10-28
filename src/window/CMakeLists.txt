qt_add_library(quickshell-window STATIC
	proxywindow.cpp
	windowinterface.cpp
	panelinterface.cpp
	floatingwindow.cpp
	popupwindow.cpp
)

qt_add_qml_module(quickshell-window
	URI Quickshell._Window
	VERSION 0.1
)

target_link_libraries(quickshell-window PRIVATE ${QT_DEPS} Qt6::QuickPrivate)

qs_pch(quickshell-window)
qs_pch(quickshell-windowplugin)

target_link_libraries(quickshell PRIVATE quickshell-windowplugin)

if (BUILD_TESTING)
	add_subdirectory(test)
endif()
