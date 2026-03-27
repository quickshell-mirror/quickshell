qs_p += {
	'Quickshell.Services.UPower': {
		'sources': files(
			'core.cpp',
			'device.cpp',
			'powerprofiles.cpp',
		),
		'moc_headers': files(
			'core.hpp',
			'device.hpp',
			'powerprofiles.hpp',
		),
		'depends_imports': ['Quickshell'],
		'dependencies': dep_qt6dbus,
		'include_directories': [include_directories('.'), root_inc],
		'cpp_pch': pch_dbus,
		'dbus_interfaces': {
			'dbus_upower_service': {
				'file': files('org.freedesktop.UPower.xml'),
				'classname': 'DBusUPowerService',
				'includes': ['-N'],
			},
			'dbus_upower_device': {
				'file': files('org.freedesktop.UPower.Device.xml'),
				'classname': 'DBusUPowerDevice',
				'includes': ['-N'],
			},
		},
	},
}
