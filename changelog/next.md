## Other Changes

- Added support for Hyprland's new workspace address format.

## New Features

- Added `WaylandWindow.appId` for setting the app id of toplevel windows on Wayland.

## Bug Fixes

- Fixed main process crashes on pam subprocess misbehavior.
- Fixed crashes when attempting to create or modify session locks reentrantly.
- Fixed networking state breaking after restarting NetworkManager.
- Fixed accessibility information not being provided for Quickshell windows.
- Fixed `WlSessionLockSurface.screen` being null and never updating with a real screen on monitor plug.
- Fixed `NotificationAction.text` not accepting text updates.
