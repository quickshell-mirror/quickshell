qs_wl_src += files('../session_lock.cpp')
qs_wl_moc += files('../session_lock.hpp')

qs_wl_p += {
	'quickshell-wayland-sessionlock': {
		'sources': files(
			'lock.cpp',
			'manager.cpp',
			'session_lock.cpp',
			'shell_integration.cpp',
			'surface.cpp',
		),
		'moc_headers': files(
			'lock.hpp',
			'session_lock.hpp',
		),
		'moc_only': true,
		'include_directories': include_directories('.'),
		'protos': ['ext-session-lock-v1'],
	},
}
