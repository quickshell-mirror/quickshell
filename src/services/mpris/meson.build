qs_p += {
	'Quickshell.Services.Mpris': {
		'sources': files(
			'player.cpp',
			'watcher.cpp',
		),
		'moc_headers': files(
			'player.hpp',
			'watcher.hpp',
		),
		'dependencies': dep_qt6dbus,
		'include_directories': [include_directories('.'), root_inc],
		'cpp_pch': pch_dbus,
		'dbus_interfaces': {
			'dbus_player': {
				'file': files('org.mpris.MediaPlayer2.Player.xml'),
				'classname': 'DBusMprisPlayer',
				'includes': ['-N'],
			},
			'dbus_player_app': {
				'file': files('org.mpris.MediaPlayer2.xml'),
				'classname': 'DBusMprisPlayerApp',
				'includes': ['-N'],
			},
		},
	},
}
