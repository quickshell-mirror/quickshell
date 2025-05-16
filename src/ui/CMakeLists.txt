qt_add_library(quickshell-ui STATIC
	reload_popup.cpp
)

# do not install this module
qt_add_qml_module(quickshell-ui
	URI Quickshell._InternalUi
	VERSION 0.1
	DEPENDENCIES QtQuick
	QML_FILES
		Tooltip.qml
		ReloadPopup.qml
)

qs_module_pch(quickshell-ui SET large)

target_link_libraries(quickshell-ui PRIVATE Qt::Quick)
target_link_libraries(quickshell PRIVATE quickshell-uiplugin)
