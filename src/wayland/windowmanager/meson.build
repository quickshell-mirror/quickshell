qs_wl_p += {
	'quickshell-wayland-windowsystem': {
		'sources': files(
			'ext_workspace.cpp',
			'windowmanager.cpp',
			'windowset.cpp',
		),
		'moc_headers': files(
			'ext_workspace.hpp',
			'windowmanager.hpp',
			'windowset.hpp',
		),
		'moc_only': true,
		'include_directories': include_directories('.'),
		'protos': ['ext-workspace-v1'],
	},
}

qs_link_whole += static_library(
	'quickshell-wayland-windowsystem-init',
	files('init.cpp'),
	dependencies: dep_qt6,
)
