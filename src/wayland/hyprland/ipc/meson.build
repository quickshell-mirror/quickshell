module_src = [
	files(
		'connection.cpp',
		'monitor.cpp',
		'qml.cpp',
		'workspace.cpp',
	),
]
module_hpp = [
	files(
		'connection.hpp',
		'monitor.hpp',
		'qml.hpp',
		'workspace.hpp',
	),
]
module_protos = ['hyprland-toplevel-mapping-v1']

if get_option('wl_toplevel_management').allowed()
	module_src += files(
		'hyprland_toplevel.cpp',
		'toplevel_mapping.cpp',
	)
	module_hpp += files(
		'hyprland_toplevel.hpp',
		'toplevel_mapping.hpp',
	)
	module_protos += 'wlr-foreign-toplevel-management-unstable-v1'
endif

qs_hypr_p += {
	'Quickshell.Hyprland._Ipc': {
		'sources': module_src,
		'moc_headers': module_hpp,
		'include_directories': include_directories('.'),
		'protos': module_protos,
	},
}
