qs_hypr_p = {}

hypr_mods = {
	'Focus Grabbing': ['focus_grab', 'hyprland_focus_grab'],
	'Global Shortcuts': ['global_shortcuts', 'hyprland_global_shortcuts'],
	'Hyprland IPC': ['ipc', 'hyprland_ipc'],
	'Surface Extensions': ['surface', 'hyprland_surface_extensions'],
}

foreach d, m : hypr_mods
	opt = get_option(m[1])
	sum_optdeps += {'  ' + d: {'opt': opt, 'section': 'Wayland'}}
	if opt.allowed()
		subdir(m[0])
	endif
endforeach

hyprland_modules = []
foreach name, props : qs_hypr_p
	hyprland_modules += name
	props += {'skip_mod': true}
	qs_wl_p += {name: props}
endforeach

if hyprland_modules.length() > 0
	qs_wl_p += {
		'Quickshell.Hyprland': {
			'imports': hyprland_modules,
			'include_directories': include_directories('.'),
		},
	}
endif
