add_library(quickshell-build INTERFACE)

if (NOT DEFINED GIT_REVISION)
	execute_process(
		COMMAND git rev-parse HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_REVISION
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endif()

if (CRASH_REPORTER)
	set(CRASH_REPORTER_DEF 1)
else()
	set(CRASH_REPORTER_DEF 0)
endif()

if (DISTRIBUTOR_DEBUGINFO_AVAILABLE)
	set(DEBUGINFO_AVAILABLE 1)
else()
	set(DEBUGINFO_AVAILABLE 0)
endif()

configure_file(build.hpp.in build.hpp @ONLY ESCAPE_QUOTES)

target_include_directories(quickshell-build INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
