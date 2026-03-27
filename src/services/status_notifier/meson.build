qs_p += {
	'Quickshell.Services.SystemTray': {
		'sources': files(
			'dbus_item_types.cpp',
			'host.cpp',
			'item.cpp',
			'qml.cpp',
			'watcher.cpp',
		),
		'moc_headers': files(
			'host.hpp',
			'item.hpp',
			'qml.hpp',
			'watcher.hpp',
		),
		'dependencies': dep_qt6dbus,
		'include_directories': [include_directories('.'), root_inc],
		'cpp_pch': pch_dbus,
		'dbus_interfaces': {
			'dbus_watcher': {
				'file': files('org.kde.StatusNotifierWatcher.xml'),
				'classname': 'StatusNotifierWatcherAdaptor',
				'includes': ['-l', 'qs::service::sni::StatusNotifierWatcher', '-i', files('watcher.hpp')],
				'adaptor': true,
			},
			'dbus_item': {
				'file': files('org.kde.StatusNotifierItem.xml'),
				'classname': 'DBusStatusNotifierItem',
				'includes': ['-i', files('dbus_item_types.hpp')],
			},
			'dbus_watcher_interface': {
				'file': files('org.kde.StatusNotifierWatcher.xml'),
				'classname': 'DBusStatusNotifierWatcher',
			},
		},
	},
}
