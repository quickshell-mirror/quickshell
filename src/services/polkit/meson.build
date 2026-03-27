qs_p += {
	'Quickshell.Services.Polkit': {
		'sources': files(
			'agentimpl.cpp',
			'flow.cpp',
			'identity.cpp',
			'listener.cpp',
			'qml.cpp',
			'session.cpp',
		),
		'moc_headers': files(
			'agentimpl.hpp',
			'flow.hpp',
			'identity.hpp',
			'qml.hpp',
			'session.hpp',
		),
		'dependencies': deps,
		'include_directories': [include_directories('.')],
		'cpp_pch': pch_common,
	},
}
