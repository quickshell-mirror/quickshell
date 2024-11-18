qt_add_library(quickshell-widgets STATIC
	cliprect.cpp
)

qt_add_qml_module(quickshell-widgets
	URI Quickshell.Widgets
	VERSION 0.1
	QML_FILES
		IconImage.qml
		ClippingRectangle.qml
)

qt6_add_shaders(quickshell-widgets "widgets-cliprect"
	NOHLSL NOMSL BATCHABLE PRECOMPILE OPTIMIZED QUIET
	PREFIX "/Quickshell/Widgets"
	FILES shaders/cliprect.frag
	OUTPUTS shaders/cliprect.frag.qsb
)

qt6_add_shaders(quickshell-widgets "widgets-cliprect-ub"
	NOHLSL NOMSL BATCHABLE PRECOMPILE OPTIMIZED QUIET
	PREFIX "/Quickshell/Widgets"
	FILES shaders/cliprect.frag
	OUTPUTS shaders/cliprect-ub.frag.qsb
	DEFINES CONTENT_UNDER_BORDER
)

install_qml_module(quickshell-widgets)

qs_module_pch(quickshell-widgets)

target_link_libraries(quickshell-widgets PRIVATE Qt::Quick)
target_link_libraries(quickshell PRIVATE quickshell-widgetsplugin)
