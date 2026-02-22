qt_add_library(quickshell-network-freebsd STATIC
	backend.cpp
)

target_link_libraries(quickshell-network-freebsd PRIVATE Qt::Core Qt::Qml)
target_include_directories(quickshell-network-freebsd PRIVATE ..)
