tests = {
	'datastream': 'datastream',
	'process': 'process',
}

foreach desc, src : tests
	qs_t += {
		'exe': src,
		'suite': 'io',
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
