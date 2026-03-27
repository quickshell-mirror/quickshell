qs_p += {
	'Quickshell.Services.Pam': {
		'sources': files(
			'conversation.cpp',
			'ipc.cpp',
			'qml.cpp',
			'subprocess.cpp',
		),
		'moc_headers': files(
			'conversation.hpp',
			'qml.hpp',
		),
		'dependencies': deps,
		'include_directories': [include_directories('.'), root_inc],
	},
}
