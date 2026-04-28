wl_screencopy_conf = configuration_data({'SCREENCOPY': true})

sc_mods = {
	'  Hyprland': {
		'dir': 'hyprland_screencopy',
		'opt': 'wl_screencopy_hyprland_toplevel',
		'opt_deps': [get_option('hyprland')],
	},
	'  Image Copy Capture': {
		'dir': 'image_copy_capture',
		'opt': 'wl_screencopy_icc',
	},
	'  wlroots': {'dir': 'wlr_screencopy', 'opt': 'wl_screencopy_wlr'},
}

foreach desc, mod : sc_mods
	opt = get_option(mod['opt'])
	sum_optdeps += {desc: mod + {'opt': opt, 'section': 'Wayland'}}
	foreach o : mod.get('opt_deps', [])
		opt = opt.require(o.allowed())
	endforeach
	if 'var' in mod
		set_variable(mod['var'], opt.allowed())
	endif
	if opt.allowed()
		subdir(mod['dir'])
	endif
endforeach

build_hpp = configure_file(
	input: 'build.hpp.in',
	output: 'build.hpp',
	configuration: wl_screencopy_conf,
	format: 'cmake',
)

module_name = 'Quickshell.Wayland._Screencopy'
qs_wl_modules += module_name

qs_wl_p += {
	module_name: {
		'sources': files(
			'manager.cpp',
			'view.cpp',
		),
		'moc_headers': files(
			'manager.hpp',
			'view.hpp',
		),
		'include_directories': include_directories('.'),
		'dependencies': deps,
	},
}
