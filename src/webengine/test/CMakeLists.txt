add_executable(webengine webengine.cpp)
target_link_libraries(webengine PRIVATE Qt::Quick Qt::Test)
add_test(NAME webengine WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" COMMAND $<TARGET_FILE:webengine>)