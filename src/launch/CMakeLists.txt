find_package(CLI11 CONFIG REQUIRED)

qt_add_library(quickshell-launch STATIC
	parsecommand.cpp
	command.cpp
	launch.cpp
	main.cpp
)

target_link_libraries(quickshell-launch PRIVATE
	Qt::Quick Qt::Widgets CLI11::CLI11 quickshell-build
)

qs_add_pchset(launch
	DEPENDENCIES Qt::Core CLI11::CLI11
	HEADERS
		<CLI/App.hpp>
		<qcoreapplication.h>
)

qs_pch(quickshell-launch SET launch)

target_link_libraries(quickshell PRIVATE quickshell-launch)
