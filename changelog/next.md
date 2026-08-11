## New Features

### Niri IPC module

Added the `Quickshell.Niri` module, providing live niri compositor state over IPC:
workspaces, windows, monitors, screencasts, overview state, and keyboard layouts,
plus action dispatch (`Niri.dispatch`) and a raw event stream (`Niri.rawEvent`).

## Other Changes

- Added support for Hyprland's new workspace address format.

## Bug Fixes

- Fixed main process crashes on pam subprocess misbehavior.
- Fixed crashes when attempting to create or modify session locks reentrantly.
- Fixed networking state breaking after restarting NetworkManager.
- Fixed accessibility information not being provided for Quickshell windows.
- Fixed `WlSessionLockSurface.screen` being null and never updating with a real screen on monitor plug.
- Fixed `NotificationAction.text` not accepting text updates.
- Fixed PipeWire route volume writes for devices without a reported volume step.
- Fixed a crash when quitting or reloading while a `Loader` was still loading asynchronously.
- Fixed future `Socket` connection attempts after a failed connection attempt.
