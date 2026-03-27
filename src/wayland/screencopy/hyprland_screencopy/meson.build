wl_screencopy_conf.set('SCREENCOPY_HYPRLAND_TOPLEVEL', opt.allowed())

qs_link += static_library(
	'quickshell-wayland-screencopy-hyprland',
	files('hyprland_screencopy.cpp'),
	dependencies: qs_wl_deps,
	link_with: proto_link['hyprland-toplevel-export-v1'],
	include_directories: qs_wl_inc,
)
