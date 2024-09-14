qt_add_library(quickshell-widgets STATIC)

qt_add_qml_module(quickshell-widgets
	URI Quickshell.Widgets
	VERSION 0.1
	QML_FILES
		IconImage.qml
)

qs_pch(quickshell-widgets)
qs_pch(quickshell-widgetsplugin)

target_link_libraries(quickshell PRIVATE quickshell-widgetsplugin)
