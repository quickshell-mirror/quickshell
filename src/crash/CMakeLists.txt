qt_add_library(quickshell-crash STATIC
	main.cpp
	interface.cpp
	handler.cpp
)

qs_pch(quickshell-crash SET large)

find_package(PkgConfig REQUIRED)
pkg_check_modules(breakpad REQUIRED IMPORTED_TARGET breakpad)
# only need client?? take only includes from pkg config todo
target_link_libraries(quickshell-crash PRIVATE PkgConfig::breakpad -lbreakpad_client)

# quick linked for pch compat
target_link_libraries(quickshell-crash PRIVATE quickshell-build Qt::Quick Qt::Widgets)

target_link_libraries(quickshell PRIVATE quickshell-crash)
