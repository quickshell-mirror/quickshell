qt_add_library(quickshell-dwl STATIC
	output.cpp
	tag.cpp
	manager.cpp
	qml.cpp
)

qt_add_qml_module(quickshell-dwl
	URI Quickshell.DWL
	VERSION 0.1
)

install_qml_module(quickshell-dwl)

wl_proto(wlp-dwl-ipc dwl-ipc-unstable-v2 "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(quickshell-dwl PRIVATE
	Qt::Core
	Qt::Gui
	Qt::GuiPrivate
	Qt::WaylandClient
	Qt::WaylandClientPrivate
	Wayland::Client
	wlp-dwl-ipc
)

target_link_libraries(quickshell PRIVATE quickshell-dwlplugin)
