qs_p += {
	'Quickshell.Io': {
		'sources': [
			files(
				'datastream.cpp',
				'fileview.cpp',
				'ipc.cpp',
				'ipccomm.cpp',
				'ipchandler.cpp',
				'jsonadapter.cpp',
				'process.cpp',
				'processcore.cpp',
			),
			get_option('sockets').allowed() ? files('socket.cpp') : [],
		],
		'qml_sources': files('FileView.qml'),
		'moc_headers': [
			files(
				'datastream.hpp',
				'fileview.hpp',
				'ipccomm.hpp',
				'ipchandler.hpp',
				'jsonadapter.hpp',
				'process.hpp',
				'processcore.hpp',
			),
			get_option('sockets').allowed() ? files('socket.hpp') : [],
		],
		'include_directories': [include_directories('.'), root_inc],
		'cpp_pch': files('pch/pch.hpp')[0].full_path(),
	},
}

if get_option('tests').allowed()
	subdir('test')
endif
