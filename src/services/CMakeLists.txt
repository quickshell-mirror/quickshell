if (SERVICE_STATUS_NOTIFIER)
	add_subdirectory(status_notifier)
endif()

if (SERVICE_PIPEWIRE)
	add_subdirectory(pipewire)
endif()

if (SERVICE_MPRIS)
	add_subdirectory(mpris)
endif()

if (SERVICE_PAM)
	add_subdirectory(pam)
endif()

if (SERVICE_GREETD)
	add_subdirectory(greetd)
endif()

if (SERVICE_UPOWER)
	add_subdirectory(upower)
endif()

if (SERVICE_NOTIFICATIONS)
	add_subdirectory(notifications)
endif()
