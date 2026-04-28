qs_wl_modules = []
qs_wl_moc = []
qs_wl_defs = []
qs_wl_src = []
qs_wl_inc = []
qs_wl_p = {}

opt = get_option('wayland')
dep_qt6wayland = dependency(
	'qt6',
	modules: ['WaylandClient'],
	private_headers: true,
	required: opt,
)
sum_qmods += dep_qt6wayland.found() ? 'WaylandClient' : []

have_clientpriv = cxx.has_header(
	'private/qwaylandinputdevice_p.h',
	dependencies: dep_qt6wayland,
	required: opt,
)
sum_qmods += (have_clientpriv ? 'WaylandClientPrivate' : [])

# NOTE: Qt people forgot to add .pc files for WaylandGlobal but we can probably
# avoid dependency('Qt6WaylandClientPrivate') (for which Meson would call CMake)
qwl_inc = dep_qt6.get_variable(pkgconfig: 'includedir')
qwl_inc = qwl_inc / 'QtWaylandGlobal' / dep_qt6.version()
if fs.is_dir(qwl_inc)
	qs_wl_inc += include_directories(qwl_inc)
endif
have_wlpriv = cxx.has_header(
	'QtWaylandGlobal/private/qtwaylandglobal-config_p.h',
	include_directories: qs_wl_inc,
	required: opt,
)
sum_qmods += (have_wlpriv ? 'WaylandGlobalPrivate' : [])

dep_wl_client = dependency('wayland-client', required: opt)
dep_wl_proto = dependency('wayland-protocols', required: opt)
dep_xkbcommon = dependency('xkbcommon', required: opt)
wayland = import('wayland', required: false)

qs_wl_deps = [dep_wl_client, dep_wl_proto, dep_qt6wayland, dep_xkbcommon]

have_wl = have_clientpriv and have_wlpriv and wayland.found()
foreach d : qs_wl_deps
	if not d.found()
		have_wl = false
	endif
endforeach

if opt.allowed() != false
	sum_deps += {
		dep_wl_proto.name(): dep_wl_proto,
		dep_wl_client.name(): dep_wl_client,
		dep_xkbcommon.name(): dep_xkbcommon,
	}
endif
if not have_wl
	if opt.allowed() != false
		sum_optdeps += {
			'Available': {
				'opt': opt,
				'deps': qs_wl_deps,
				'section': 'Wayland',
				'missing': (have_wlpriv ? [] : [
					'missing: Qt WaylandGlobalPrivate',
				]) + (have_clientpriv ? [] : [
					'missing: Qt WaylandClientPrivate',
				]),
			},
		}
	endif
	subdir_done()
endif

qs_wl_deps += dep_qt6
waylandscanner = find_program('wayland-scanner')
qtwaylandscanner = find_program(
	'qtwaylandscanner',
	dirs: ['/usr/libexec/qt6', '/usr/lib/qt6', '/usr/lib64/qt6'],
)

subdir('private')

mods = {
	'wlroots Layer Shell': {
		'dir': 'wlr_layershell',
		'opt': 'wl_wlr_layershell',
	},
	'Session Lock': {
		'dir': 'session_lock',
		'opt': 'wl_session_lock',
	},
	'Foreign Toplevel Management': {
		'dir': 'toplevel_management',
		'opt': 'wl_toplevel_management',
	},
	'Hyprland': {
		'dir': 'hyprland',
		'opt': 'hyprland',
	},
	'Screencopy': {
		'dir': 'screencopy',
		'opt': 'wl_screencopy',
		'var': 'have_screencopy',
		'deps': [
			dep_drm,
			dependency('egl', required: opt),
			dependency('gbm', required: opt),
			dependency('glesv2', required: opt),
		],
	},
}

foreach desc, mod : mods
	opt = get_option(mod['opt'])
	deps = mod.get('deps', [])
	sum_optdeps += {desc: mod + {'opt': opt, 'section': 'Wayland'}}
	foreach d : deps
		opt = opt.require(d.found())
		sum_deps += {d.name(): d}
	endforeach
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

subdir('buffer')
subdir('background_effect')
subdir('idle_inhibit')
subdir('idle_notify')
subdir('shortcuts_inhibit')
subdir('windowmanager')

wl_pch = files('pch/pch.hpp')[0].full_path()

foreach name, props : qs_wl_p
	if not props.get('moc_only', false) and not props.get('skip_mod', false)
		qs_wl_modules += name
	endif
	link = []
	foreach l : props.get('protos', [])
		link += proto_link[l]
	endforeach
	qs_p += {
		name: {
			'version': props.get('version', '0.1'),
			'sources': props.get('sources', []),
			'moc_headers': props.get('moc_headers', []),
			'moc_only': props.get('moc_only', false),
			'imports': props.get('imports', []),
			'depends_imports': ['Quickshell', 'Quickshell.Wayland'] + props.get(
				'depends_imports',
				[],
			),
			'dependencies': [qs_wl_deps, props.get('dependencies', [])],
			'include_directories': [
				qs_wl_inc,
				props.get('include_directories', []),
			],
			'link_with': [link, props.get('link_with', [])],
			'cpp_pch': props.get('cpp_pch', wl_pch),
			'cpp_args': [qs_wl_defs, props.get('cpp_args', [])],
		},
	}
endforeach

module_name = 'Quickshell.Wayland'
qs_p += {
	module_name: {
		'sources': [
			files(
				'output_tracking.cpp',
				'platformmenu.cpp',
				'popupanchor.cpp',
				'util.cpp',
				'wl_proxy_safe_deref.cpp',
				'xdgshell.cpp',
			),
			qs_wl_src,
		],
		'moc_headers': [files('output_tracking.hpp'), qs_wl_moc],
		'imports': qs_wl_modules,
		'dependencies': qs_wl_deps,
		'include_directories': [include_directories('.'), qs_wl_inc],
		'link_with': proto_link['xdg-shell'],
		'cpp_pch': wl_pch,
		'cpp_args': qs_wl_defs,
		# required for wl_proxy_safe_deref
		'link_args': '-Wl,--export-dynamic-symbol=wl_proxy_get_listener',
	},
}

qs_link_whole += static_library(
	'quickshell-wayland-init',
	'init.cpp',
	dependencies: dep_qt6,
	cpp_args: qs_wl_defs,
)
