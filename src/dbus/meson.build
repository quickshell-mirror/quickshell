opt_dbus = get_option('dbus')
dep_qt6dbus = dependency('qt6', modules: ['DBus'], required: opt_dbus)

sum_optdeps += {
	'DBus': {
		'opt': opt_dbus,
		'section': 'Internal',
		'deps': [dep_qt6dbus],
	},
}

if not dep_qt6dbus.found()
	subdir_done()
endif
sum_qmods += 'DBus'

qdbusxml2cpp = find_program(
	'qdbusxml2cpp',
	'qdbusxml2cpp6',
	dep_qt6dbus.get_variable(pkgconfig: 'bindir') / 'qdbusxml2cpp',
)

qs_p += {
	'quickshell-dbus': {
		'sources': files(
			'bus.cpp',
			'objectmanager.cpp',
			'properties.cpp',
		),
		'moc_headers': files(
			'objectmanager.hpp',
			'properties.hpp',
		),
		'moc_deps': dep_qt6dbus,
		'moc_only': true,
		'dependencies': dep_qt6dbus,
		'include_directories': [include_directories('.'), root_inc],
		'cpp_pch': pch_dbus,
		'dbus_interfaces': {
			'dbus_properties': {
				'file': files('org.freedesktop.DBus.Properties.xml'),
				'classname': 'DBusPropertiesInterface',
			},
			'dbus_objectmanager': {
				'file': files('org.freedesktop.DBus.ObjectManager.xml'),
				'classname': 'DBusObjectManagerInterface',
				'includes': ['-i', 'dbus_objectmanager_types.hpp'],
			},
		},
	},
}

subdir('dbusmenu')
