# Build instructions
Instructions for building from source and distro packagers. We highly recommend
distro packagers read through this page fully.

## Dependencies
Quickshell has a set of base dependencies you will always need, names vary by distro:

- `cmake`
- `qt6base`
- `qt6declarative`
- `pkg-config`

We recommend an implicit dependency on `qt6svg`. If it is not installed, svg images and
svg icons will not work, including system ones.

At least Qt 6.6 is required.

All features are enabled by default and some have their own dependencies.

##### Additional note to packagers:
If your package manager supports enabling some features but not others,
we recommend not exposing the subfeatures and just the main ones that introduce
new dependencies: `wayland`, `x11`, `pipewire`, `hyprland`

### Jemalloc
We recommend leaving Jemalloc enabled as it will mask memory fragmentation caused
by the QML engine, which results in much lower memory usage. Without this you
will get a perceived memory leak.

To disable: `-DUSE_JEMALLOC=OFF`

Dependencies: `jemalloc`

### Unix Sockets
This feature allows interaction with unix sockets and creating socket servers
which is useful for IPC and has no additional dependencies.

WARNING: Disabling unix sockets will NOT make it safe to run arbitrary code using quickshell.
There are many vectors which mallicious code can use to escape into your system.

To disable: `-DSOCKETS=OFF`

### Wayland
This feature enables wayland support. Subfeatures exist for each particular wayland integration.

WARNING: Wayland integration relies on features that are not part of the public Qt API and which
may break in minor releases. Updating quickshell's dependencies without ensuring without ensuring
that the current Qt version is supported WILL result in quickshell failing to build or misbehaving
at runtime.

Currently supported Qt versions: `6.6`, `6.7`.

To disable: `-DWAYLAND=OFF`

Dependencies:
 - `qt6wayland`
 - `wayland` (libwayland-client)
 - `wayland-scanner` (may be part of your distro's wayland package)
 - `wayland-protocols`

#### Wlroots Layershell
Enables wlroots layershell integration through the [wlr-layer-shell-unstable-v1] protocol,
enabling use cases such as bars overlays and backgrounds.
This feature has no extra dependencies.

To disable: `-DWAYLAND_WLR_LAYERSHELL=OFF`

[wlr-layer-shell-unstable-v1]: https://wayland.app/protocols/wlr-layer-shell-unstable-v1

#### Session Lock
Enables session lock support through the [ext-session-lock-v1] protocol,
which allows quickshell to be used as a session lock under compatible wayland compositors.

[ext-session-lock-v1]: https://wayland.app/protocols/ext-session-lock-v1

### X11
This feature enables x11 support. Currently this implements panel windows for X11 similarly
to the wlroots layershell above.

To disable: `-DX11=OFF`

Dependencies: `libxcb`

### Pipewire
This features enables viewing and management of pipewire nodes.

To disable: `-DSERVICE_PIPEWIRE=OFF`

Dependencies: `libpipewire`

### StatusNotifier / System Tray
This feature enables system tray support using the status notifier dbus protocol.

To disable: `-DSERVICE_STATUS_NOTIFIER=OFF`

Dependencies: `qt6dbus` (usually part of qt6base)

### MPRIS
This feature enables access to MPRIS compatible media players using its dbus protocol.

To disable: `-DSERVICE_MPRIS=OFF`

Dependencies: `qt6dbus` (usually part of qt6base)

### Hyprland
This feature enables hyprland specific integrations. It requires wayland support
but has no extra dependencies.

To disable: `-DHYPRLAND=OFF`

#### Hyprland Global Shortcuts
Enables creation of global shortcuts under hyprland through the [hyprland-global-shortcuts-v1]
protocol. Generally a much nicer alternative to using unix sockets to implement the same thing.
This feature has no extra dependencies.

To disable: `-DHYPRLAND_GLOBAL_SHORTCUTS=OFF`

[hyprland-global-shortcuts-v1]: https://github.com/hyprwm/hyprland-protocols/blob/main/protocols/hyprland-global-shortcuts-v1.xml

#### Hyprland Focus Grab
Enables windows to grab focus similarly to a context menu undr hyprland through the
[hyprland-focus-grab-v1] protocol. This feature has no extra dependencies.

To disable: `-DHYPRLAND_FOCUS_GRAB=OFF`

[hyprland-focus-grab-v1]: https://github.com/hyprwm/hyprland-protocols/blob/main/protocols/hyprland-focus-grab-v1.xml

## Building
*For developers and prospective contributors: See [CONTRIBUTING.md](CONTRIBUTING.md).*

We highly recommend using `ninja` to run the build, but you can use makefiles if you must.

#### Configuring the build
```sh
$ cmake -GNinja -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo [additional disable flags from above here]
```

Note that features you do not supply dependencies for MUST be disabled with their associated flags
or quickshell will fail to build.

Additionally, note that clang builds much faster than gcc if you care.

You may disable debug information but it's only a couple megabytes and is extremely helpful
for helping us fix problems when they do arise.

#### Building
```sh
$ cmake --build build
```

#### Installing
```sh
$ cmake --install build
```
