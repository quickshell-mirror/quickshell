set(INPUT_METHOD_PRINT_KETS OFF)

qt_add_library(quickshell-wayland-input-method STATIC
	input_method.cpp
	keyboard_grab.cpp
	virtual_keyboard.cpp
	key_map_state.cpp
	manager.cpp
	qml.cpp
	qml_helpers.cpp
	c_helpers.cpp
	types.cpp
)

target_compile_definitions(quickshell-wayland-input-method PRIVATE INPUT_METHOD_PRINT=0)

qt_add_qml_module(quickshell-wayland-input-method
	URI Quickshell.Wayland._InputMethod
	VERSION 0.1
	DEPENDENCIES QtQml
)

qs_add_module_deps_light(quickshell-wayland-input-method
	Quickshell Quickshell.Wayland
)

install_qml_module(quickshell-wayland-input-method)

wl_proto(zwp-input-method input-method-unstable-v2 "${CMAKE_CURRENT_SOURCE_DIR}")
wl_proto(zwp-virtual-keyboard virtual-keyboard-unstable-v1 "${CMAKE_CURRENT_SOURCE_DIR}")
wl_proto(zwp-text-input text-input-unstable-v3 "${WAYLAND_PROTOCOLS}/unstable/text-input")

target_link_libraries(quickshell-wayland-input-method PRIVATE
	Qt::Quick Qt::WaylandClient Qt::WaylandClientPrivate wayland-client
	zwp-input-method zwp-virtual-keyboard zwp-text-input
)

qs_module_pch(quickshell-wayland-input-method SET large)

target_link_libraries(quickshell PRIVATE quickshell-wayland-input-methodplugin)
