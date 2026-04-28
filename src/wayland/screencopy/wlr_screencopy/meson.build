wl_screencopy_conf.set('SCREENCOPY_WLR', opt.allowed())

qs_link += static_library(
	'quickshell-wayland-screencopy-wlr',
	files('wlr_screencopy.cpp'),
	dependencies: qs_wl_deps,
	link_with: proto_link['wlr-screencopy-unstable-v1'],
	include_directories: qs_wl_inc,
)
