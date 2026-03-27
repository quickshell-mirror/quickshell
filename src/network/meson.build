opt = get_option('network')
sum_optdeps += {
	'Network': {
		'opt': opt,
		'section': 'Management',
		'opt_deps': [opt_dbus],
	},
}
if not opt.require(opt_dbus.allowed()).allowed()
	subdir_done()
endif

qs_p += {
	'Quickshell.Networking': {
		'sources': files(
			'device.cpp',
			'network.cpp',
			'wifi.cpp',
		),
		'moc_headers': files(
			'device.hpp',
			'network.hpp',
			'wifi.hpp',
		),
		'dependencies': dep_qt6dbus,
		'include_directories': [
			include_directories('.'),
			include_directories('nm'),
		],
		'cpp_pch': pch_dbus,
	},
}

subdir('nm')
