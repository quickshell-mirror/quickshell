dep_cli11 = dependency('CLI11')
sum_deps += {dep_cli11.name(): dep_cli11}

qs_link += static_library(
	'quickshell-launch',
	files(
		'command.cpp',
		'launch.cpp',
		'main.cpp',
		'parsecommand.cpp',
	),
	dependencies: [
		dep_qt6,
		dep_cli11,
	],
	include_directories: build_inc,
	cpp_pch: files('pch/pch.hpp')[0].full_path(),
)
