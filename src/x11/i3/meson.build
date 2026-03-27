opt = get_option('i3')
sum_optdeps += {'i3': {'opt': opt, 'section': 'X11'}}
if not opt.allowed()
	subdir_done()
endif

i3_modules = []
subdir('ipc')

if i3_modules.length() > 0
	qs_p += {
		'Quickshell.I3': {
			'imports': i3_modules,
			'include_directories': include_directories('.'),
		},
	}
endif
