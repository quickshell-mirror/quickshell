qs_p += {
	'quickshell-network-nm': {
		'sources': files(
			'accesspoint.cpp',
			'backend.cpp',
			'connection.cpp',
			'device.cpp',
			'utils.cpp',
			'wireless.cpp',
		),
		'moc_headers': files(
			'accesspoint.hpp',
			'backend.hpp',
			'connection.hpp',
			'device.hpp',
			'enums.hpp',
			'utils.hpp',
			'wireless.hpp',
		),
		'moc_only': true,
		'dependencies': dep_qt6dbus,
		'include_directories': [include_directories('.'), root_inc],
		'cpp_pch': pch_dbus,
		'dbus_interfaces': {
			'dbus_nm_backend': {
				'file': files('org.freedesktop.NetworkManager.xml'),
				'classname': 'DBusNetworkManagerProxy',
				'includes': ['-N', '-i', files('dbus_types.hpp')],
			},
			'dbus_nm_device': {
				'file': files('org.freedesktop.NetworkManager.Device.xml'),
				'classname': 'DBusNMDeviceProxy',
				'includes': ['-N'],
			},
			'dbus_nm_wireless': {
				'file': files('org.freedesktop.NetworkManager.Device.Wireless.xml'),
				'classname': 'DBusNMWirelessProxy',
				'includes': ['-N'],
			},
			'dbus_nm_accesspoint': {
				'file': files('org.freedesktop.NetworkManager.AccessPoint.xml'),
				'classname': 'DBusNMAccessPointProxy',
				'includes': ['-N'],
			},
			'dbus_nm_connection_settings': {
				'file': files('org.freedesktop.NetworkManager.Settings.Connection.xml'),
				'classname': 'DBusNMConnectionSettingsProxy',
				'includes': ['-N', '-i', files('dbus_types.hpp')],
			},
			'dbus_nm_active_connection': {
				'file': files('org.freedesktop.NetworkManager.Connection.Active.xml'),
				'classname': 'DBusNMActiveConnectionProxy',
				'includes': ['-N'],
			},
		},
	},
}
