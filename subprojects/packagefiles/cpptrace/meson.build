project(
	'cpptrace',
	'cpp',
	version: '1.0.4',
	default_options: {'cpp_std': 'c++11'},
)

defs = [
	'-DCPPTRACE_GET_SYMBOLS_WITH_LIBDWARF',
	'-DCPPTRACE_DEMANGLE_WITH_CXXABI',
	'-DCPPTRACE_UNWIND_WITH_LIBUNWIND',
	'-DCPPTRACE_HAS_CXX_EXCEPTION_TYPE',
	'-DCPPTRACE_STATIC_DEFINE',
	'-DHAS_ATTRIBUTE_PACKED',
	'-DNOMINMAX',
	'-DUNW_LOCAL_ONLY',
]

cxx = meson.get_compiler('cpp')
dl = dependency('dl')
deps = [dependency('libdwarf'), dependency('libunwind'), dl]
if cxx.has_header_symbol('dlfcn.h', 'dl_find_object', dependencies: dl)
	defs += '-DCPPTRACE_HAS_DL_FIND_OBJECT'
endif

cpptrace_inc = include_directories('include', 'src')
cpptrace_dep = declare_dependency(
	link_with: static_library(
		'cpptrace',
		files(
			'include/cpptrace/cpptrace.hpp',
			'include/ctrace/ctrace.h',
			'src/binary/elf.cpp',
			'src/binary/mach-o.cpp',
			'src/binary/module_base.cpp',
			'src/binary/object.cpp',
			'src/binary/pe.cpp',
			'src/binary/safe_dl.cpp',
			'src/cpptrace.cpp',
			'src/ctrace.cpp',
			'src/demangle/demangle_with_cxxabi.cpp',
			'src/demangle/demangle_with_nothing.cpp',
			'src/demangle/demangle_with_winapi.cpp',
			'src/exceptions.cpp',
			'src/formatting.cpp',
			'src/from_current.cpp',
			'src/jit/jit_objects.cpp',
			'src/logging.cpp',
			'src/options.cpp',
			'src/platform/dbghelp_utils.cpp',
			'src/platform/memory_mapping.cpp',
			'src/prune_symbol.cpp',
			'src/snippets/snippet.cpp',
			'src/symbols/dwarf/debug_map_resolver.cpp',
			'src/symbols/dwarf/dwarf_options.cpp',
			'src/symbols/dwarf/dwarf_resolver.cpp',
			'src/symbols/symbols_core.cpp',
			'src/symbols/symbols_with_addr2line.cpp',
			'src/symbols/symbols_with_dbghelp.cpp',
			'src/symbols/symbols_with_dl.cpp',
			'src/symbols/symbols_with_libbacktrace.cpp',
			'src/symbols/symbols_with_libdwarf.cpp',
			'src/symbols/symbols_with_nothing.cpp',
			'src/unwind/unwind_with_dbghelp.cpp',
			'src/unwind/unwind_with_execinfo.cpp',
			'src/unwind/unwind_with_libunwind.cpp',
			'src/unwind/unwind_with_nothing.cpp',
			'src/unwind/unwind_with_unwind.cpp',
			'src/unwind/unwind_with_winapi.cpp',
			'src/utils.cpp',
			'src/utils/error.cpp',
			'src/utils/io/file.cpp',
			'src/utils/io/memory_file_view.cpp',
			'src/utils/microfmt.cpp',
			'src/utils/replace_all.cpp',
			'src/utils/string_view.cpp',
			'src/utils/utils.cpp',
		),
		cpp_args: defs,
		include_directories: cpptrace_inc,
		dependencies: deps,
	),
	compile_args: ['-DCPPTRACE_STATIC_DEFINE'],
	include_directories: cpptrace_inc,
	dependencies: deps,
)

meson.override_dependency('cpptrace', cpptrace_dep)
