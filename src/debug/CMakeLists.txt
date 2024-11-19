qt_add_library(quickshell-debug STATIC
	lint.cpp
)

qs_pch(quickshell-debug)
target_link_libraries(quickshell-debug PRIVATE Qt::Quick)
target_link_libraries(quickshell PRIVATE quickshell-debug)
