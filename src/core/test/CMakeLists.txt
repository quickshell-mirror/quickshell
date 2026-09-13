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

# Exercise translation loading in the real shell.
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)
add_executable(translations translations.cpp)
target_link_libraries(translations PRIVATE Qt::Core Qt::Test)
target_compile_definitions(translations PRIVATE QS_BINARY="$<TARGET_FILE:quickshell>")
add_dependencies(translations quickshell)
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/qml_ru.qm"
	COMMAND Qt6::lrelease "${CMAKE_CURRENT_SOURCE_DIR}/translations/qml_ru.ts"
		-qm "${CMAKE_CURRENT_BINARY_DIR}/qml_ru.qm"
	DEPENDS "translations/qml_ru.ts"
	VERBATIM
)
target_sources(translations PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/qml_ru.qm")
add_test(NAME translations WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" COMMAND translations)
set_tests_properties(translations PROPERTIES TIMEOUT 30)
