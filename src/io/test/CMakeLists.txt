function (qs_test name)
	add_executable(${name} ${ARGN})
	target_link_libraries(${name} PRIVATE ${QT_DEPS} Qt6::Test)
	add_test(NAME ${name} WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" COMMAND $<TARGET_FILE:${name}>)
endfunction()

qs_test(datastream datastream.cpp ../datastream.cpp)
