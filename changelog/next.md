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
- Added the ability to override Quickshell.cacheDir with a custom path.

## Other Changes

- IPC operations filter available instances to the current display connection by default.

## Bug Fixes

- Fixed volume control breaking with pipewire pro audio mode.
- Fixed escape sequence handling in desktop entries.
- Fixed volumes not initializing if a pipewire device was already loaded before its node.

## Packaging Changes

`glib` and `polkit` have been added as dependencies when compiling with polkit agent support.
