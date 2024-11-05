qt_add_library(quickshell-widgets STATIC)

qt_add_qml_module(quickshell-widgets
	URI Quickshell.Widgets
	VERSION 0.1
	QML_FILES
		IconImage.qml
)

install_qml_module(quickshell-widgets)

qs_module_pch(quickshell-widgets)

target_link_libraries(quickshell PRIVATE quickshell-widgetsplugin)
