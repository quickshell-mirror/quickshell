opt_polkit = get_option('service_polkit')

services = {
	'greetd': {'dir': 'greetd', 'opt': 'service_greetd'},
	'MPRIS': {
		'dir': 'mpris',
		'opt': 'service_mpris',
		'opt_deps': [opt_dbus],
	},
	'Notifications': {
		'dir': 'notifications',
		'opt': 'service_notifications',
		'opt_deps': [opt_dbus],
	},
	'PAM': {
		'dir': 'pam',
		'opt': 'service_pam',
		'deps': [dependency('pam')],
	},
	'PipeWire': {
		'dir': 'pipewire',
		'opt': 'service_pipewire',
		'deps': [
			dependency(
				'libpipewire-0.3',
				required: get_option('service_pipewire'),
			),
		],
	},
	'Polkit': {
		'dir': 'polkit',
		'opt': 'service_polkit',
		'deps': [
			dependency('glib-2.0', version: '>=2.36', required: opt_polkit),
			dependency('gobject-2.0', required: opt_polkit),
			dependency('polkit-agent-1', required: opt_polkit),
			dependency('polkit-gobject-1', required: opt_polkit),
		],
	},
	'Status Notifiers (System Tray)': {
		'dir': 'status_notifier',
		'opt': 'service_status_notifier',
		'opt_deps': [opt_dbus],
	},
	'UPower': {'dir': 'upower', 'opt': 'service_upower', 'opt_deps': [opt_dbus]},
}

foreach desc, svc : services
	opt = get_option(svc['opt'])
	deps = svc.get('deps', [])
	sum_optdeps += {desc: svc + {'opt': opt, 'section': 'Services'}}
	foreach d : deps
		opt = opt.require(d.found())
	endforeach
	foreach o : svc.get('opt_deps', [])
		opt = opt.require(o.allowed())
	endforeach
	if opt.allowed()
		subdir(svc['dir'])
	endif
endforeach
