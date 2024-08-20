find_package(CLI11 CONFIG REQUIRED)

qt_add_library(quickshell-core STATIC
	main.cpp
	plugin.cpp
	shell.cpp
	variants.cpp
	rootwrapper.cpp
	proxywindow.cpp
	reload.cpp
	rootwrapper.cpp
	qmlglobal.cpp
	qmlscreen.cpp
	region.cpp
	persistentprops.cpp
	windowinterface.cpp
	floatingwindow.cpp
	panelinterface.cpp
	popupwindow.cpp
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
	crashinfo.cpp
	common.cpp
)

if (CRASH_REPORTER)
	set(CRASH_REPORTER_DEF 1)
endif()

add_library(quickshell-build INTERFACE)
configure_file(build.hpp.in build.hpp)
target_include_directories(quickshell-build INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(quickshell-core PRIVATE quickshell-build)

qt_add_qml_module(quickshell-core URI Quickshell VERSION 0.1)

target_link_libraries(quickshell-core PRIVATE ${QT_DEPS} Qt6::QuickPrivate CLI11::CLI11)
qs_pch(quickshell-core)

target_link_libraries(quickshell PRIVATE quickshell-coreplugin)

if (BUILD_TESTING)
	add_subdirectory(test)
endif()
