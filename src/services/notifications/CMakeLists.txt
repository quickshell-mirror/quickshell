qt_add_dbus_adaptor(DBUS_INTERFACES
	org.freedesktop.Notifications.xml
	server.hpp
	qs::service::notifications::NotificationServer
	dbus_notifications
	DBusNotificationServer
)

qt_add_library(quickshell-service-notifications STATIC
	server.cpp
	notification.cpp
	dbusimage.cpp
	qml.cpp
	${DBUS_INTERFACES}
)

# dbus headers
target_include_directories(quickshell-service-notifications PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

qt_add_qml_module(quickshell-service-notifications
	URI Quickshell.Services.Notifications
	VERSION 0.1
	DEPENDENCIES QtQml Quickshell
)

target_link_libraries(quickshell-service-notifications PRIVATE ${QT_DEPS} quickshell-dbus)
target_link_libraries(quickshell PRIVATE quickshell-service-notificationsplugin)

qs_pch(quickshell-service-notifications)
qs_pch(quickshell-service-notificationsplugin)
