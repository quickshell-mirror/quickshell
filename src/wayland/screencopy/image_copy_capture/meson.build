wl_screencopy_conf.set('SCREENCOPY_ICC', opt.allowed())

qs_link += static_library(
	'quickshell-wayland-screencopy-icc',
	files(
		'image_copy_capture.cpp',
	),
	dependencies: qs_wl_deps,
	link_with: [
		proto_link['ext-foreign-toplevel-list-v1'],
		proto_link['ext-image-copy-capture-v1'],
		proto_link['ext-image-capture-source-v1'],
	],
	include_directories: qs_wl_inc,
)
