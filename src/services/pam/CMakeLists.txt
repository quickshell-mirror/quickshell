qt_add_library(quickshell-service-pam STATIC
	qml.cpp
	conversation.cpp
	ipc.cpp
	subprocess.cpp
)

qt_add_qml_module(quickshell-service-pam
	URI Quickshell.Services.Pam
	VERSION 0.1
	DEPENDENCIES QtQml
)

install_qml_module(quickshell-service-pam)

target_link_libraries(quickshell-service-pam PRIVATE
	Qt::Qml pam ${PAM_LIBRARIES}
	Qt::Quick # pch
)

qs_module_pch(quickshell-service-pam)

target_link_libraries(quickshell PRIVATE quickshell-service-pamplugin)
