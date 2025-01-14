qt_add_library(quickshell-wayland-screencopy-icc STATIC
	image_copy_capture.cpp
)

wl_proto(wlp-ext-foreign-toplevel ext-foreign-toplevel-list-v1 "${WAYLAND_PROTOCOLS}/staging/ext-foreign-toplevel-list")
wl_proto(wlp-image-copy-capture ext-image-copy-capture-v1 "${WAYLAND_PROTOCOLS}/staging/ext-image-copy-capture")
wl_proto(wlp-image-capture-source ext-image-capture-source-v1 "${WAYLAND_PROTOCOLS}/staging/ext-image-capture-source")

target_link_libraries(quickshell-wayland-screencopy-icc PRIVATE
	Qt::WaylandClient Qt::WaylandClientPrivate wayland-client
	Qt::Quick # for pch
)

target_link_libraries(quickshell-wayland-screencopy-icc PUBLIC
	wlp-image-copy-capture wlp-image-capture-source
	wlp-ext-foreign-toplevel # required for capture source to build
)

qs_pch(quickshell-wayland-screencopy-icc SET large)
