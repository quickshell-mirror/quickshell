if (SERVICE_STATUS_NOTIFIER)
	add_subdirectory(status_notifier)
endif()

if (SERVICE_PIPEWIRE)
	add_subdirectory(pipewire)
endif()
