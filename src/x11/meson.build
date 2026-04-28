opt = get_option('x11')
dep_xcb = dependency('xcb', required: opt)
if opt.allowed() != false
	sum_deps += {dep_xcb.name(): dep_xcb}
endif
if not dep_xcb.found()
	if opt.allowed() != false
		sum_optdeps += {
			'Available': {
				'opt': opt,
				'deps': [dep_xcb],
				'section': 'X11',
			},
		}
	endif
	subdir_done()
endif

qs_p += {
	'Quickshell.X11': {
		'sources': files(
			'panel_window.cpp',
			'util.cpp',
		),
		'moc_headers': files('panel_window.hpp'),
		'dependencies': dep_xcb,
		'include_directories': include_directories('.'),
	},
}

subdir('i3')

qs_link_whole += static_library(
	'quickshell-x11-init',
	files('init.cpp'),
	dependencies: [
		dep_qt6,
		dep_xcb,
	],
)

