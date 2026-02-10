## Breaking Changes

### Config paths are no longer canonicalized

This fixes nix configs changing shell-ids on rebuild as the shell id is now derived from
the symlink path. Configs with a symlink in their path will have a different shell id.

Shell ids are used to derive the default config / state / cache folders, so those files
will need to be manually moved if using a config behind a symlinked path without an explicitly
set shell id.

## New Features

- Added support for creating Polkit agents.
- Added support for creating wayland idle inhibitors.
- Added support for wayland idle timeouts.
- Added support for inhibiting wayland compositor shortcuts for focused windows.
- Added the ability to override Quickshell.cacheDir with a custom path.
- Added minimized, maximized, and fullscreen properties to FloatingWindow.
- Added the ability to handle move and resize events to FloatingWindow.
- Pipewire service now reconnects if pipewire dies or a protocol error occurs.
- Added pipewire audio peak detection.
- Added initial support for network management.
- Added support for grabbing focus from popup windows.
- Added support for IPC signal listeners.
- Added Quickshell version checking and version gated preprocessing.
- Added a way to detect if an icon is from the system icon theme or not.

## Other Changes

- FreeBSD is now partially supported.
- IPC operations filter available instances to the current display connection by default.
- PwNodeLinkTracker ignores sound level monitoring programs.

## Bug Fixes

- Fixed volume control breaking with pipewire pro audio mode.
- Fixed volume control breaking with bluez streams and potentially others.
- Fixed escape sequence handling in desktop entries.
- Fixed volumes not initializing if a pipewire device was already loaded before its node.
- Fixed hyprland active toplevel not resetting after window closes.
- Fixed hyprland ipc window names and titles being reversed.
- Fixed missing signals for system tray item title and description updates.
- Fixed asynchronous loaders not working after reload.
- Fixed asynchronous loaders not working before window creation.
- Fixed memory leak in IPC handlers.
- Fixed ClippingRectangle related crashes.

## Packaging Changes

`glib` and `polkit` have been added as dependencies when compiling with polkit agent support.
`vulkan-headers` has been added as a build-time dependency for screencopy (Vulkan backend support).
