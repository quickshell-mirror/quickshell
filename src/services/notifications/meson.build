qs_p += {
	'Quickshell.Services.Notifications': {
		'sources': files(
			'dbusimage.cpp',
			'notification.cpp',
			'qml.cpp',
			'server.cpp',
		),
		'moc_headers': files(
			'notification.hpp',
			'qml.hpp',
			'server.hpp',
		),
		'depends_imports': ['Quickshell'],
		'dependencies': dep_qt6dbus,
		'include_directories': [include_directories('.'), root_inc],
		'cpp_pch': pch_dbus,
		'dbus_interfaces': {
			'dbus_notifications': {
				'file': files('org.freedesktop.Notifications.xml'),
				'classname': 'DBusNotificationServer',
				'includes': ['-l', 'qs::service::notifications::NotificationServer', '-i', files('server.hpp')],
				'adaptor': true,
			},
		},
	},
}
