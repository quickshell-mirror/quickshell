if not have_screencopy
	subdir_done()
endif

qs_wl_p += {
	'quickshell-wayland-buffer': {
		'sources': files('dmabuf.cpp', 'manager.cpp', 'shm.cpp'),
		'moc_headers': files('manager.hpp'),
		'moc_only': true,
		'include_directories': include_directories('.'),
		'dependencies': dep_drm,
		'protos': ['linux-dmabuf-v1'],
	},
}
