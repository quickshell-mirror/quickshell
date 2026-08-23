## Bug Fixes

- Fixed main process crashes on pam subprocess misbehavior.
- Fixed Hyprland.activeToplevel being null until the user changes focus, by seeding it from j/clients during init.
