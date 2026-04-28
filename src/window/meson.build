qs_p += {
	'Quickshell._Window': {
		'sources': files(
			'floatingwindow.cpp',
			'panelinterface.cpp',
			'popupwindow.cpp',
			'proxywindow.cpp',
			'windowinterface.cpp',
		),
		'moc_headers': files(
			'floatingwindow.hpp',
			'panelinterface.hpp',
			'popupwindow.hpp',
			'proxywindow.hpp',
			'windowinterface.hpp',
		),
		'depends_imports': ['Quickshell'],
		'include_directories': include_directories('.'),
		'cpp_pch': files('pch/pch.hpp')[0].full_path(),
	},
}

qs_link_whole += static_library(
	'quickshell-window-init',
	files('init.cpp'),
	dependencies: dep_qt6,
)

if get_option('tests').allowed()
	subdir('test')
endif
