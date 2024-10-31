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

add_library(quickshell-window-init OBJECT init.cpp)

target_link_libraries(quickshell-window PRIVATE ${QT_DEPS} Qt6::QuickPrivate)
target_link_libraries(quickshell-windowplugin PRIVATE ${QT_DEPS})
target_link_libraries(quickshell-window-init PRIVATE ${QT_DEPS})

qs_pch(quickshell-window)
qs_pch(quickshell-windowplugin)
qs_pch(quickshell-window-init)

target_link_libraries(quickshell PRIVATE quickshell-windowplugin quickshell-window-init)

if (BUILD_TESTING)
	add_subdirectory(test)
endif()
