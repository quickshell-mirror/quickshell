# List of targets linked into quickshell executable
qs_link = []
qs_link_whole = []
# Extra sources and include_directories
qs_src = []
qs_inc = []
# QML plugins
qs_p = {}
# Tests
qs_t = []

# Shader compiler
qt6_qml_path = dep_qt6.get_variable(pkgconfig: 'libdir') / 'qt6' / 'qml'
qsb = find_program(
	'qsb',
	'qsb6',
	dep_qt6.get_variable(pkgconfig: 'bindir') / 'qsb',
	required: true,
)

build_inc = include_directories('build')
subdir('crash')  # Crash handler
subdir('build')  # Build information header
subdir('launch')  # Application entry point
subdir('debug')

subdir('core')
subdir('ipc')
subdir('io')
subdir('window')  # Windows and panels
subdir('widgets')  # Shader compiler needed here
subdir('ui')  # Internal UI

subdir('windowmanager')
subdir('wayland')
subdir('x11')

subdir('dbus')
subdir('bluetooth')  # BlueZ D-Bus interface
subdir('network')  # NetworkManager
subdir('services')  # Various external desktop session services

# This only looks bad if you haven’t seen the Qt CMake macros that
# do basically the same thing except in OVER NINE THOUSAND LINES OF CODE
# instead of just over a hundred. Most of their code is workarounds for
# CMake’s limitations, such as not even having real lists and generally
# behaving a lot like an MS Batch clone.
qs_mods = {}
foreach module_name, props : qs_p
	# FIXME: The Qt6 module cannot generate DBus interface/adaptor code yet, so we do it ourselves.
	dbus_cpp = []
	foreach name, dprops : props.get('dbus_interfaces', {})
		dbus_out = custom_target(
			name,
			input: dprops['file'],
			output: [name + '.h', name + '.cpp'],
			command: [
				qdbusxml2cpp,
				'@INPUT@',
				dprops.get('includes', []),
				'-m',
				['-c', dprops['classname']],
				dprops.get('adaptor', false) ? '-a' : '-p',
				'@OUTPUT0@:@OUTPUT1@',
			],
		)
		dbus_cpp += dbus_out[1]
		dbus_cpp += qt6.compile_moc(
			headers: dbus_out[0],
			dependencies: dep_qt6dbus,
		)
	endforeach

	# FIXME: The Qt6 module cannot generate plugin code yet, so we do it ourselves.

	module_id = module_name.underscorify()
	# We need some extra processing to make the symbols and resources linkable.
	moc_only = props.get('moc_only', false)
	if not moc_only
		# Meson doesn’t embed the raw QML if qmlcache is enabled (default),
		# so this should be enough.
		r_keep = 'QT_KEEP_RESOURCE(qmlcache_@0@)'.format(module_id)
		r_ext = 'QT_DECLARE_EXTERN_RESOURCE(qmlcache_@0@)'.format(module_id)
		conf = {
			'NAME': module_id,
			'KEEP_RESOURCES': 'qml_sources' in props ? r_keep : '',
			'EXTERN_RESOURCES': 'qml_sources' in props ? r_ext : '',
		}
		plugin = configure_file(
			input: support / 'qplugin.cpp.in',
			output: module_id + 'Plugin.cpp',
			configuration: conf,
		)
		qs_src += configure_file(
			input: support / 'qplugin-init.cpp.in',
			output: module_id + '-plugin-init.cpp',
			configuration: conf,
		)

		# FIXME: The Qt6 module cannot generate shader resources yet, so we do it ourselves.
		foreach s : props.get('shaders', [])
			name = s['path'].replace('/', '_')
			qsb_out = custom_target(
				name,
				input: s['file'],
				output: name,
				depfile: name + '.d',
				command: [
					qsb,
					['--glsl', '100es,120,150'],
					'-b',
					'-O',
					'-s',
					s.get('defs', []),
					['-o', '@OUTPUT@'],
					['--depfile', '@DEPFILE@'],
					'@INPUT@',
				],
				install: true,
				install_dir: qt6_qml_path / s['prefix'],
			)
			qs_src += [
				qsb_out,
				qt6.compile_resources(
					sources: configure_file(
						input: support / 'qresource.qrc.in',
						output: name + '.qrc',
						configuration: {
							'file': qsb_out.full_path(),
							'prefix': s['prefix'],
							'path': s['path'],
						},
					),
				),
			]
		endforeach
	endif

	# That’s the actual QML plugin target
	mod = static_library(
		module_id,
		props.get('sources', []),
		dbus_cpp,
		moc_only ? qt6.compile_moc(
			sources: props.get('moc_sources', []),
			headers: props.get('moc_headers', []),
			dependencies: props.get('moc_deps', []),
		) : [
			plugin,
			qt6.qml_module(
				module_name,
				version: props.get('version', '0.1'),
				qml_sources: props.get('qml_sources', []),
				moc_headers: props.get('moc_headers', []),
				dependencies: [dep_qt6, props.get('moc_deps', [])],
				imports: props.get('imports', []),
				depends_imports: props.get('depends_imports', []),
				default_imports: props.get('default_imports', []),
				optional_imports: props.get('optional_imports', []),
			),
			qt6.compile_moc(
				sources: [plugin],
				dependencies: dep_qt6,
				extra_args: ['-Muri=' + module_name],
			),
		],
		dependencies: [dep_qt6, props.get('dependencies', [])],
		link_with: props.get('link_with', []),
		include_directories: props.get('include_directories', []),
		cpp_args: [
			['-DQT_PLUGIN', '-DQT_STATICPLUGIN'],
			props.get('cpp_args', []),
		],
		cpp_pch: props.get('cpp_pch', []),
		link_args: props.get('link_args', []),
	)
	qs_link += mod
	qs_mods += {module_name: mod}
endforeach

summary('Test Suite', dep_qt6test.found(), section: 'Build', bool_yn: true)
if dep_qt6test.found()
	foreach t : qs_t
		links = [t.get('link_with', []), qs_mod_debug]
		foreach m : t['link_mods']
			links += qs_mods[m]
		endforeach
		test(
			t['desc'],
			executable(
				'_'.join(['test', t['suite'], t['exe']]).underscorify(),
				t['sources'],
				qt6.compile_moc(
					headers: t['moc_headers'],
					dependencies: dep_qt6,
				),
				dependencies: [dep_qt6, dep_qt6test, malloc],
				link_with: links,
			),
			args: ['-tap'],
			protocol: 'tap',
			suite: t['suite'],
		)
	endforeach
endif

quickshell = executable(
	'quickshell',
	'main.cpp',
	qs_src,
	dependencies: [dep_qt6, malloc],
	link_with: qs_link,
	link_whole: qs_link_whole,
	include_directories: qs_inc,
	install: true,
)
