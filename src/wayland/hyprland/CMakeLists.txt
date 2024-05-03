qt_add_library(quickshell-hyprland STATIC)
qt_add_qml_module(quickshell-hyprland URI Quickshell.Hyprland VERSION 0.1)

add_subdirectory(focus_grab)

target_link_libraries(quickshell-hyprland PRIVATE ${QT_DEPS})

qs_pch(quickshell-hyprland)
qs_pch(quickshell-hyprlandplugin)

target_link_libraries(quickshell PRIVATE quickshell-hyprlandplugin)
