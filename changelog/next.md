## Bug Fixes

- Fixed ScreencopyView not displaying when only lock surfaces are shown.
- Fixed WlSessionLockSurface.visible crashing if accessed before backing surface creation.
- Fixed mpris players returning `rate` for `minRate` and `maxRate`.
- Fixed missing/wrong change signals on various properties.
- Fixed session lock crashes on sleep, wake, DPMS, and unlocking.
