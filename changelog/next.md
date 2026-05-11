## Bug Fixes

- Fixed ScreencopyView not displaying when only lock surfaces are shown.
- Fixed WlSessionLockSurface.visible crashing if accessed before backing surface creation.
- Fixed Hyprland.activeToplevel being null until the user changes focus, by seeding it from j/clients during init.
