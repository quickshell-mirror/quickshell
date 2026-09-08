add_executable(sessionlocksurface session_lock.cpp)
target_link_libraries(sessionlocksurface PRIVATE
	Qt::Quick Qt::Test quickshell-wayland quickshell-window quickshell-core quickshell-ui quickshell-io
)
add_test(NAME sessionlocksurface WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" COMMAND $<TARGET_FILE:sessionlocksurface>)
