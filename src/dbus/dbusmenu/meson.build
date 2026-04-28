qs_p += {
	'Quickshell.DBusMenu': {
		'sources': files(
			'dbus_menu_types.cpp',
			'dbusmenu.cpp',
		),
		'moc_headers': files(
			'dbusmenu.hpp',
		),
		'dependencies': dep_qt6dbus,
		'include_directories': [include_directories('.'), root_inc],
		'cpp_pch': pch_dbus,
		'dbus_interfaces': {
			'dbus_menu': {
				'file': files('com.canonical.dbusmenu.xml'),
				'classname': 'DBusMenuInterface',
				'includes': ['-i', files('dbus_menu_types.hpp')],
			},
		},
	},
}
