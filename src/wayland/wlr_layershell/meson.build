qs_wl_defs += '-DQS_WAYLAND_WLR_LAYERSHELL'
qs_wl_p += {
	'Quickshell.Wayland._WlrLayerShell': {
		'sources': files(
			'shell_integration.cpp',
			'surface.cpp',
			'wlr_layershell.cpp',
		),
		'moc_headers': files('wlr_layershell.hpp'),
		'include_directories': include_directories('.'),
		'protos': ['xdg-shell', 'wlr-layer-shell-unstable-v1'],
	},
}
