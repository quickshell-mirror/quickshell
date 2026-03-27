tests = {
	'transformwatcher': 'transformwatcher',
	'ringbuffer': 'ringbuf',
	'scriptmodel': 'scriptmodel',
	'stacklist': 'stacklist',
}

foreach desc, src : tests
	qs_t += {
		'exe': src,
		'suite': 'core',
		'desc': desc,
		'sources': files(src + '.cpp'),
		'moc_headers': files(src + '.hpp'),
		'link_mods': [
			'Quickshell',
			'Quickshell._Window',
			'Quickshell._InternalUi',
			'Quickshell.Io',
		],
	}
endforeach
