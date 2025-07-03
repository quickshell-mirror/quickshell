function (qs_test name)
	add_executable(${name} ${ARGN})
	target_link_libraries(${name} PRIVATE Qt::Quick Qt::Network Qt::Test quickshell-io quickshell-core quickshell-window quickshell-ui)
	add_test(NAME ${name} WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" COMMAND $<TARGET_FILE:${name}>)
endfunction()

qs_test(datastream datastream.cpp ../datastream.cpp)
qs_test(process process.cpp ../process.cpp ../datastream.cpp ../processcore.cpp)
