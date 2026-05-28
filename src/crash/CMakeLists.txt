qt_add_library(quickshell-crash STATIC
	main.cpp
	interface.cpp
	handler.cpp
)

qs_pch(quickshell-crash SET large)

if (VENDOR_CPPTRACE)
	message(STATUS "Vendoring cpptrace...")
	include(FetchContent)

	# For use without internet access see: https://cmake.org/cmake/help/latest/module/FetchContent.html#variable:FETCHCONTENT_SOURCE_DIR_%3CuppercaseName%3E
	FetchContent_Declare(
  	cpptrace
  	GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
  	GIT_TAG        v1.0.4
	)

	set(CPPTRACE_UNWIND_WITH_LIBUNWIND TRUE)
	FetchContent_MakeAvailable(cpptrace)
else ()
	find_package(cpptrace REQUIRED)

	# useful for cross after you have already checked cpptrace is built correctly
	if (NOT DO_NOT_CHECK_CPPTRACE_USABILITY)
		try_run(CPPTRACE_SIGNAL_SAFE_UNWIND CPPTRACE_SIGNAL_SAFE_UNWIND_COMP
			SOURCE_FROM_CONTENT check.cxx "
				#include <cpptrace/basic.hpp>
				int main() {
					return cpptrace::can_signal_safe_unwind() ? 0 : 1;
				}
			"
			LOG_DESCRIPTION "Checking ${CPPTRACE_SIGNAL_SAFE_UNWIND}"
			LINK_LIBRARIES cpptrace::cpptrace
			COMPILE_OUTPUT_VARIABLE CPPTRACE_SIGNAL_SAFE_UNWIND_LOG
			CXX_STANDARD 20
			CXX_STANDARD_REQUIRED ON
		)

		if (NOT CPPTRACE_SIGNAL_SAFE_UNWIND_COMP)
			message(STATUS "${CPPTRACE_SIGNAL_SAFE_UNWIND_LOG}")
			message(FATAL_ERROR "Failed to compile cpptrace signal safe unwind tester.")
		endif()

		if (NOT CPPTRACE_SIGNAL_SAFE_UNWIND EQUAL 0)
			message(STATUS "Cpptrace signal safe unwind test exited with: ${CPPTRACE_SIGNAL_SAFE_UNWIND}")
			message(FATAL_ERROR "Cpptrace was built without CPPTRACE_UNWIND_WITH_LIBUNWIND set to true. Enable libunwind support in the package or set VENDOR_CPPTRACE to true when building Quickshell.")
		endif()
	endif ()
endif ()

# quick linked for pch compat
target_link_libraries(quickshell-crash PRIVATE quickshell-build Qt::Quick Qt::Widgets cpptrace::cpptrace)

target_link_libraries(quickshell PRIVATE quickshell-crash)
