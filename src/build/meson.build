configure_file(
	input: 'build.hpp.in',
	output: 'build.hpp',
	configuration: {
		'quickshell_VERSION': '.'.join(version_parts),
		'quickshell_VERSION_MAJOR': version_parts[0],
		'quickshell_VERSION_MINOR': version_parts[1],
		'quickshell_VERSION_PATCH': version_parts[2],
		'UNRELEASED_FEATURES': ';'.join(unreleased_features),
		'GIT_REVISION': meson.project_version(),
		'DISTRIBUTOR': get_option('distributor'),
		'CRASH_HANDLER_DEF': get_option('crash_handler').require(
			dep_cpptrace.found(),
		).allowed() ? 1 : 0,
		'CMAKE_BUILD_TYPE': get_option('buildtype'),
		'CMAKE_CXX_COMPILER_ID': cxx.get_id(),
		'CMAKE_CXX_COMPILER_VERSION': cxx.version(),
		'CMAKE_CXX_FLAGS': '',  # unavailable
		'QS_BUILD_OPTIONS': meson.build_options().replace('"', '\\"'),
		'CRASHREPORT_URL': get_option('crashreport_url'),
	},
)

qs_inc += build_inc
