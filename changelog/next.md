## Other Changes

- Added support for Hyprland's new workspace address format.

## Bug Fixes

- Fixed graphics initialization failures in the initial session lock surfaces crashing Quickshell. The lock attempt is now aborted instead.
- Fixed main process crashes on pam subprocess misbehavior.
- Fixed crashes when attempting to create or modify session locks reentrantly.
- Fixed networking state breaking after restarting NetworkManager.
- Fixed accessibility information not being provided for Quickshell windows.
- Fixed `WlSessionLockSurface.screen` being null and never updating with a real screen on monitor plug.
- Fixed `NotificationAction.text` not accepting text updates.
