qs_p += {
	'Quickshell.Widgets': {
		'sources': [
			files(
				'cliprect.cpp',
				'marginwrapper.cpp',
				'wrapper.cpp',
			),
		],
		'qml_sources': files(
			'ClippingRectangle.qml',
			'ClippingWrapperRectangle.qml',
			'ClippingWrapperRectangleInternal.qml',
			'IconImage.qml',
			'WrapperItem.qml',
			'WrapperMouseArea.qml',
			'WrapperRectangle.qml',
		),
		'moc_headers': files(
			'cliprect.hpp',
			'marginwrapper.hpp',
			'wrapper.hpp',
		),
		'include_directories': include_directories('.'),
		'shaders': [
			{
				'file': files('shaders/cliprect.frag'),
				'prefix': 'Quickshell/Widgets',
				'path': 'shaders/cliprect.frag.qsb',
			},
			{
				'file': files('shaders/cliprect.frag'),
				'defs': ['-DCONTENT_UNDER_BORDER'],
				'prefix': 'Quickshell/Widgets',
				'path': 'shaders/cliprect-ub.frag.qsb',
			},
		],
	},
}
