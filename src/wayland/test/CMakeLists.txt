function (qs_test name)
	add_executable(${name} ${ARGN})
	target_link_libraries(${name} PRIVATE
		Qt::Quick Qt::Test quickshell-wayland quickshell-window quickshell-core quickshell-ui quickshell-io
	)
	add_test(NAME ${name} WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" COMMAND $<TARGET_FILE:${name}>)
endfunction()

qs_test(sessionlock session_lock.cpp)
