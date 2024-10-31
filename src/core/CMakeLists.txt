find_package(CLI11 CONFIG REQUIRED)

qt_add_library(quickshell-core STATIC
	plugin.cpp
	shell.cpp
	variants.cpp
	rootwrapper.cpp
	reload.cpp
	rootwrapper.cpp
	qmlglobal.cpp
	qmlscreen.cpp
	region.cpp
	persistentprops.cpp
	singleton.cpp
	generation.cpp
	scan.cpp
	qsintercept.cpp
	incubator.cpp
	lazyloader.cpp
	easingcurve.cpp
	iconimageprovider.cpp
	imageprovider.cpp
	transformwatcher.cpp
	boundcomponent.cpp
	model.cpp
	elapsedtimer.cpp
	desktopentry.cpp
	objectrepeater.cpp
	platformmenu.cpp
	qsmenu.cpp
	retainable.cpp
	popupanchor.cpp
	types.cpp
	qsmenuanchor.cpp
	clock.cpp
	logging.cpp
	paths.cpp
	instanceinfo.cpp
	common.cpp
)

target_link_libraries(quickshell-core PRIVATE quickshell-build)

qt_add_qml_module(quickshell-core
	URI Quickshell
	VERSION 0.1
	DEPENDENCIES QtQuick
	OPTIONAL_IMPORTS Quickshell._Window
	DEFAULT_IMPORTS Quickshell._Window
)

target_link_libraries(quickshell-core PRIVATE ${QT_DEPS} CLI11::CLI11)

qs_pch(quickshell-core)
qs_pch(quickshell-coreplugin)

target_link_libraries(quickshell PRIVATE quickshell-coreplugin)

if (BUILD_TESTING)
	add_subdirectory(test)
endif()
