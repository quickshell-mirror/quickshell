function (qs_test name)
	add_executable(${name} ${ARGN})
	target_link_libraries(${name} PRIVATE Qt::Quick Qt::Test quickshell-core quickshell-window quickshell-ui quickshell-io)
	add_test(NAME ${name} WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" COMMAND $<TARGET_FILE:${name}>)
endfunction()

qs_test(transformwatcher transformwatcher.cpp)
qs_test(ringbuffer ringbuf.cpp)
qs_test(scriptmodel scriptmodel.cpp)
qs_test(stacklist stacklist.cpp)
qs_test(objectmodel objectmodel.cpp)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	# Keep the clock/syscall simulation isolated from the other core tests.
	qt_add_executable(clock clock.cpp ../clock.cpp)
	target_link_libraries(clock PRIVATE Qt::Core Qt::Qml Qt::Test)
	target_link_options(clock PRIVATE
		-Wl,--export-dynamic-symbol=clock_gettime
		-Wl,--wrap=timerfd_create -Wl,--wrap=timerfd_settime
		-Wl,--wrap=read -Wl,--wrap=close
	)
	add_test(NAME clock COMMAND clock)
endif()
