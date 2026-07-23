## New Features

- Added `WaylandWindow.appId` for setting the app id of toplevel windows on Wayland.

## Bug Fixes

- Fixed ScreencopyView not displaying when only lock surfaces are shown.
- Fixed WlSessionLockSurface.visible crashing if accessed before backing surface creation.
- Fixed mpris players returning `rate` for `minRate` and `maxRate`.
- Fixed missing/wrong change signals on various properties.
- Fixed session lock crashes on sleep, wake, DPMS, and unlocking.
- QsWindow.updatesEnabled makes sure windows are redrawn when set to true.
- Fixed potential crashes from usage of `WindowsetProjection.screens` during monitor unplug.
- Fixed crashes from accessing freed objects laundered through a `ScriptModel`.
- Fixed crashes when a wifi network disappear.
- Fixed unhandled notifications sending `NotificationClosed` out of order.
- Fixed `qs kill` not waiting for the process to exit.
