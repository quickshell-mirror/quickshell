## Other Changes

- The qml disk cache is now enabled. Config documents are loaded from a mirror in the runtime dir, which is visible in `Component.url` and `Qt.resolvedUrl` results.

## Bug Fixes

- Fixed main process crashes on pam subprocess misbehavior.
- Fixed crashes when attempting to create or modify session locks reentrantly.
- Fixed networking state breaking after restarting NetworkManager.
- Fixed accessibility information not being provided for Quickshell windows.
- Fixed `WlSessionLockSurface.screen` being null and never updating with a real screen on monitor plug.
- Fixed `NotificationAction.text` not accepting text updates.
