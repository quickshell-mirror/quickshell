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

target_link_libraries(quickshell-service-pam PRIVATE ${QT_DEPS} pam ${PAM_LIBRARIES})

qs_pch(quickshell-service-pam)
qs_pch(quickshell-service-pamplugin)

target_link_libraries(quickshell PRIVATE quickshell-service-pamplugin)
