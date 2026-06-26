## Bug Fixes

- Fixed ScreencopyView not displaying when only lock surfaces are shown.
- Fixed WlSessionLockSurface.visible crashing if accessed before backing surface creation.
- Fixed mpris players returning `rate` for `minRate` and `maxRate`.
- Fixed missing/wrong change signals on various properties.
- Fixed session lock crashes on sleep, wake, DPMS, and unlocking.
- QsWindow.updatesEnabled makes sure windows are redrawn when set to true.
- Fixed the polkit agent failing to register when running outside a session cgroup, such as a systemd user service, by resolving the session from XDG_SESSION_ID.
