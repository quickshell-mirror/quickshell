add_library(quickshell-build INTERFACE)

if (NOT DEFINED GIT_REVISION)
	execute_process(
		COMMAND git rev-parse HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_REVISION
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endif()

if (CRASH_HANDLER)
	set(CRASH_HANDLER_DEF 1)
else()
	set(CRASH_HANDLER_DEF 0)
endif()

if (NIX_STORE_DIR_SKIP_WATCH)
	set(NIX_STORE_DIR_SKIP_WATCH_DEF 1)
else()
	set(NIX_STORE_DIR_SKIP_WATCH_DEF 0)
endif()

configure_file(build.hpp.in build.hpp @ONLY ESCAPE_QUOTES)

target_include_directories(quickshell-build INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
