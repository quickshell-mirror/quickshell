opt = get_option('bluetooth')
sum_optdeps += {
	'Bluetooth': {
		'opt': opt,
		'section': 'Management',
		'opt_deps': [opt_dbus],
	},
}
if not opt.require(opt_dbus.allowed()).allowed()
	subdir_done()
endif

qs_p += {
	'Quickshell.Bluetooth': {
		'sources': files(
			'adapter.cpp',
			'bluez.cpp',
			'device.cpp',
		),
		'moc_headers': files(
			'adapter.hpp',
			'bluez.hpp',
			'device.hpp',
		),
		'depends_imports': ['Quickshell'],
		'dependencies': dep_qt6dbus,
		'include_directories': [include_directories('.'), root_inc],
		'cpp_pch': pch_dbus,
		'dbus_interfaces': {
			'dbus_adapter': {
				'file': files('org.bluez.Adapter.xml'),
				'classname': 'DBusBluezAdapterInterface',
			},
			'dbus_device': {
				'file': files('org.bluez.Device.xml'),
				'classname': 'DBusBluezDeviceInterface',
			},
		},
	},
}
