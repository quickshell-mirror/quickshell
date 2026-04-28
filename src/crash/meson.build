opt = get_option('crash_handler')
dep_cpptrace = dependency('cpptrace', required: opt)
sum_optdeps += {
	'Crash Handler': {
		'opt': opt,
		'section': 'Internal',
		'deps': [dep_cpptrace],
	},
}
if opt.allowed() != false
	sum_deps += {dep_cpptrace.name(): dep_cpptrace}
endif
if not dep_cpptrace.found()
	subdir_done()
endif

qs_link += static_library(
	'quickshell-crash',
	files(
		'handler.cpp',
		'interface.cpp',
		'main.cpp',
	),
	dependencies: [
		dep_qt6,
		dep_cpptrace,
	],
	include_directories: build_inc,
)
