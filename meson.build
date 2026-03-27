project(
	'quickshell',
	['c', 'cpp'],
	# `meson dist` will automatically cement the version string.
	# Resulting tarballs don’t need git.
	version: run_command('meson-support' / 'version.sh', 'get-vcs', check: true).stdout().strip(),
	license: 'LGPL-3.0-only',
	license_files: ['LICENSE', 'LICENSE-GPL'],
	default_options: {
		'cpp_std': 'gnu++20',
		'c_std': 'c23',
		'warning_level': '2',
		'buildtype': 'debugoptimized',
		'b_lto': true,
	},
	meson_version: '>=1.8.0',
)

#
# Build system foreword:
# The Meson version above does not support some of the Qt-specific preprocessing
# and associated tools that are required for Quickshell. This means we have to do
# a lot of extra work, making the result look far less elegant than it could be.
#
# Nevertheless, an attempt was made to keep the required boilerplate to a minimum
# and all the really ugly bits concentrated in one place so that they can be
# easily replaced as Meson’s Qt6 module matures.
#
# The heavy lifting is done in `src/meson.build`. It processes a dictionary of
# QtQuick module targets and sets up the test suite. The majority of build targets
# is therefore expressed as declarative data structures. If you squint hard enough,
# they look almost like real Meson functions.
#

add_project_arguments(
	'-fno-strict-aliasing',
	language: 'cpp',
)

# Chop off the git appendage
version_clean = meson.project_version().split('-')[0].strip('v')
version_parts = version_clean.split('.')

unreleased_features = [
	# string list
]

summary(
	{
		'Link-time Optimization': get_option('b_lto'),
		'Precompiled Headers': get_option('b_pch'),
		'Unreleased Features': unreleased_features.length() > 0 ? unreleased_features : '(none)',
	},
	section: 'Build',
	bool_yn: true,
)

fs = import('fs')
root_inc = include_directories('.')
support = meson.project_source_root() / 'meson-support'
cxx = meson.get_compiler('cpp')

# These dicts collect library and option dependencies for a post-setup summary.
# They are populated alongside the dependency checks, which are located mostly near
# where they are used. It’s a bunch of extra code we don’t technically need, but it
# enhances the summary display particularly for auto-features: When a feature was
# automatically disabled, it will also display a list of unmet dependencies and
# any disabled options that cause the feature to be unavailable.
sum_deps = {}
sum_optdeps = {}

qt6 = import('qt6')

# Also used for post-setup summary
sum_qmods = [
	'Core',
	'Widgets',
	'Gui',
	'Quick',
	'Qml',
	'Network',
	'ShaderTools',
]

dep_qt6 = dependency('qt6', modules: sum_qmods, private_headers: true)
if not cxx.has_header('private/qsgrenderloop_p.h', dependencies: dep_qt6)
	error('Missing Qt Quick private headers.')
endif

dep_qt6test = dependency('qt6', modules: ['Test'], required: get_option('tests'))
if dep_qt6test.found()
	sum_qmods += 'Test'
	# Enable friend class declarations to facilitate testing
	add_project_arguments('-DQS_TEST', language: 'cpp')
endif

want_malloc = get_option('malloc')
if want_malloc == 'jemalloc'
	malloc = dependency('jemalloc')
elif want_malloc == 'tcmalloc'
	malloc = dependency('libtcmalloc')
else
	malloc = disabler()
endif

summary(malloc.name(), malloc, section: 'Dependencies')
summary('Memory Allocator', want_malloc, section: 'Internal')



subdir('pch')
subdir('src')



# All done, roll credits.

sum_deps += {'Qt': [dep_qt6] + sum_qmods}
summary(sum_deps, section: 'Dependencies')

# Choosing different modules doesn’t result in different name strings,
# so let’s try to disambiguate.
depnames = {
	'Qt DBus': dep_qt6dbus,
	'Qt WaylandClient': dep_qt6wayland,
}

foreach desc, props : sum_optdeps
	missing = props.get('missing', [])
	have = props['opt'].require(missing.length() == 0)
	foreach optd : props.get('opt_deps', [])
		foreach d, p : sum_optdeps
			if p['opt'] == optd
				allowed = p['opt'].allowed()
				have = have.require(allowed)
				if not allowed
					missing += 'disabled: ' + d
				endif
				break
			endif
		endforeach
	endforeach
	foreach dep : props.get('deps', [])
		found = dep.found()
		have = have.require(found)
		if not found
			name = dep.name()
			# We only have string keys :(
			foreach n, d : depnames
				# Comparing vars of different types is a hard error,
				# hence the name checks.
				if d.name() == dep.name() and d == dep
					name = n
					break
				endif
			endforeach
			missing += 'missing: ' + name
		endif
	endforeach
	unsat = have.allowed() != props['opt'].allowed()
	summary(
		desc,
		[have.allowed()] + (unsat ? missing : []),
		section: props.get('section', ''),
		bool_yn: true,
	)
endforeach
