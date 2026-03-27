project(
	'tcmalloc',
	'cpp',
	version: '2.18.1',
	default_options: {'warning_level': '0'},
)

add_project_arguments('-DENABLE_EMERGENCY_MALLOC', language: 'cpp')
tcmalloc_inc = include_directories('generic-config', 'src', 'src/base')
tcmalloc_dep = declare_dependency(
	link_with: static_library(
		'tcmalloc',
		files(
			'src/base/dynamic_annotations.cc',
			'src/base/elf_mem_image.cc',
			'src/base/generic_writer.cc',
			'src/base/logging.cc',
			'src/base/low_level_alloc.cc',
			'src/base/proc_maps_iterator.cc',
			'src/base/spinlock.cc',
			'src/base/spinlock_internal.cc',
			'src/base/sysinfo.cc',
			'src/base/vdso_support.cc',
			'src/central_freelist.cc',
			'src/common.cc',
			'src/emergency_malloc.cc',
			'src/heap-checker-stub.cc',
			'src/heap-profile-table.cc',
			'src/heap-profiler.cc',
			'src/internal_logging.cc',
			'src/malloc_backtrace.cc',
			'src/malloc_extension.cc',
			'src/malloc_hook.cc',
			'src/memfs_malloc.cc',
			'src/page_heap.cc',
			'src/safe_strerror.cc',
			'src/sampler.cc',
			'src/span.cc',
			'src/stack_trace_table.cc',
			'src/stacktrace.cc',
			'src/static_vars.cc',
			'src/system-alloc.cc',
			'src/tcmalloc.cc',
			'src/thread_cache.cc',
			'src/thread_cache_ptr.cc',
		),
		include_directories: tcmalloc_inc,
	),
)

meson.override_dependency('libtcmalloc', tcmalloc_dep)
