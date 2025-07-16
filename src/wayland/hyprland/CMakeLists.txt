qt_add_library(quickshell-hyprland STATIC)

target_link_libraries(quickshell-hyprland PRIVATE ${QT_DEPS})

set(HYPRLAND_MODULES)

if (HYPRLAND_IPC)
	add_subdirectory(ipc)
	list(APPEND HYPRLAND_MODULES Quickshell.Hyprland._Ipc)
endif()

if (HYPRLAND_FOCUS_GRAB)
	add_subdirectory(focus_grab)
	list(APPEND HYPRLAND_MODULES Quickshell.Hyprland._FocusGrab)
endif()

if (HYPRLAND_GLOBAL_SHORTCUTS)
	add_subdirectory(global_shortcuts)
	list(APPEND HYPRLAND_MODULES Quickshell.Hyprland._GlobalShortcuts)
endif()

if (HYPRLAND_SURFACE_EXTENSIONS)
	add_subdirectory(surface)
	list(APPEND HYPRLAND_MODULES Quickshell.Hyprland._SurfaceExtensions)
endif()

qt_add_qml_module(quickshell-hyprland
	URI Quickshell.Hyprland
	VERSION 0.1
	IMPORTS ${HYPRLAND_MODULES}
)

qs_add_module_deps_light(quickshell-io Quickshell)
install_qml_module(quickshell-hyprland)

# intentionally no pch as the module is empty

target_link_libraries(quickshell PRIVATE quickshell-hyprlandplugin)
