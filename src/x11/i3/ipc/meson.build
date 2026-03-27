opt = get_option('i3_ipc')
sum_optdeps += {'  i3 IPC': {'opt': opt, 'section': 'X11'}}
if not opt.allowed()
	subdir_done()
endif

qs_p += {
	'Quickshell.I3._Ipc': {
		'sources': files(
			'connection.cpp',
			'controller.cpp',
			'listener.cpp',
			'monitor.cpp',
			'qml.cpp',
			'workspace.cpp',
		),
		'moc_headers': files(
			'connection.hpp',
			'controller.hpp',
			'listener.hpp',
			'monitor.hpp',
			'qml.hpp',
			'workspace.hpp',
		),
		'include_directories': include_directories('.'),
		'cpp_pch': pch_common,
	},
}
