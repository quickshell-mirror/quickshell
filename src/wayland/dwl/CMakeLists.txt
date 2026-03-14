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

target_precompile_headers(quickshell-dwl PRIVATE
	<cstdint>
	<qlogging.h>
	<qlist.h>
	<qobject.h>
	<qstring.h>
	<qlist.h>
	<qtmetamacros.h>
	<qtypes.h>
	<qwaylandclientextension.h>
	<qscreen.h>
	<qpa/qplatformscreen.h>
	<qpa/qplatformnativeinterface.h>
)

target_link_libraries(quickshell-dwl PRIVATE
	Qt6::Core
	Qt6::Gui
	Qt6::GuiPrivate
	Qt6::Widgets
	Qt6::WaylandClient
	Qt6::WaylandClientPrivate
	Wayland::Client
	wlp-dwl-ipc
)

target_link_libraries(quickshell PRIVATE
	quickshell-dwl
)
