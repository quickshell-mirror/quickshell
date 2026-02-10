# Build instructions
Instructions for building from source and distro packagers. We highly recommend
distro packagers read through this page fully.

## Packaging
If you are packaging quickshell for official or unofficial distribution channels,
such as a distro package repository, user repository, or other shared build location,
please set the following CMake flags.

`-DDISTRIBUTOR="your distribution platform"`

Please make this descriptive enough to identify your specific package, for example:
- `Official Nix Flake`
- `AUR (quickshell-git)`
- `Nixpkgs`
- `Fedora COPR (errornointernet/quickshell)`

`-DDISTRIBUTOR_DEBUGINFO_AVAILABLE=YES/NO`

If we can retrieve binaries and debug information for the package without actually running your
distribution (e.g. from an website), and you would like to strip the binary, please set this to `YES`.

If we cannot retrieve debug information, please set this to `NO` and
**ensure you aren't distributing stripped (non debuggable) binaries**.

In both cases you should build with `-DCMAKE_BUILD_TYPE=RelWithDebInfo` (then split or keep the debuginfo).

### QML Module dir
Currently all QML modules are statically linked to quickshell, but this is where
tooling information will go.

`-DINSTALL_QML_PREFIX="path/to/qml"`

`-DINSTALL_QMLDIR="/full/path/to/qml"`

`INSTALL_QML_PREFIX` works the same as `INSTALL_QMLDIR`, except it prepends `CMAKE_INSTALL_PREFIX`. You usually want this.

## Dependencies
Quickshell has a set of base dependencies you will always need, names vary by distro:

- `cmake`
- `qt6base`
- `qt6declarative`
- `qtshadertools` (build-time)
- `spirv-tools` (build-time)
- `pkg-config` (build-time)
- `cli11` (static library)

Build time dependencies and static libraries don't have to exist at runtime,
however build time dependencies must be compiled for the architecture of
the builder, while static libraries must be compiled for the architecture
of the target.

On some distros, private Qt headers are in separate packages which you may have to install.
We currently require private headers for the following libraries:

- `qt6declarative`
- `qt6wayland` (for Qt versions prior to 6.10)

We recommend an implicit dependency on `qt6svg`. If it is not installed, svg images and
svg icons will not work, including system ones.

At least Qt 6.6 is required.

All features are enabled by default and some have their own dependencies.

### Crash Reporter
The crash reporter catches crashes, restarts quickshell when it crashes,
and collects useful crash information in one place. Leaving this enabled will
enable us to fix bugs far more easily.

To disable: `-DCRASH_REPORTER=OFF`

Dependencies: `google-breakpad` (static library)

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
 - `qt6wayland` (for Qt versions prior to 6.10)
 - `wayland` (libwayland-client)
 - `wayland-scanner` (build time)
 - `wayland-protocols` (static library)

Note that one or both of `wayland-scanner` and `wayland-protocols` may be bundled
with you distro's wayland package.

#### Wlroots Layershell
Enables wlroots layershell integration through the [zwlr-layer-shell-v1] protocol,
enabling use cases such as bars overlays and backgrounds.
This feature has no extra dependencies.

To disable: `-DWAYLAND_WLR_LAYERSHELL=OFF`

[zwlr-layer-shell-v1]: https://wayland.app/protocols/wlr-layer-shell-unstable-v1

#### Session Lock
Enables session lock support through the [ext-session-lock-v1] protocol,
which allows quickshell to be used as a session lock under compatible wayland compositors.

To disable: `-DWAYLAND_SESSION_LOCK=OFF`

[ext-session-lock-v1]: https://wayland.app/protocols/ext-session-lock-v1


#### Foreign Toplevel Management
Enables management of windows of other clients through the [zwlr-foreign-toplevel-management-v1] protocol,
which allows quickshell to be used as a session lock under compatible wayland compositors.

[zwlr-foreign-toplevel-management-v1]: https://wayland.app/protocols/wlr-foreign-toplevel-management-unstable-v1

To disable: `-DWAYLAND_TOPLEVEL_MANAGEMENT=OFF`

#### Screencopy
Enables streaming video from monitors and toplevel windows through various protocols.

To disable: `-DSCREENCOPY=OFF`

Dependencies:
- `libdrm`
- `libgbm`
- `vulkan-headers` (build-time)

Specific protocols can also be disabled:
- `DSCREENCOPY_ICC=OFF` - Disable screencopy via [ext-image-copy-capture-v1]
- `DSCREENCOPY_WLR=OFF` - Disable screencopy via [zwlr-screencopy-v1]
- `DSCREENCOPY_HYPRLAND_TOPLEVEL=OFF` - Disable screencopy via [hyprland-toplevel-export-v1]

[ext-image-copy-capture-v1]:https://wayland.app/protocols/ext-image-copy-capture-v1
[zwlr-screencopy-v1]: https://wayland.app/protocols/wlr-screencopy-unstable-v1
[hyprland-toplevel-export-v1]: https://wayland.app/protocols/hyprland-toplevel-export-v1

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

### PAM
This feature enables PAM integration for user authentication.

To disable: `-DSERVICE_PAM=OFF`

Dependencies: `pam`

### Polkit
This feature enables creating Polkit agents that can prompt user for authentication.

To disable: `-DSERVICE_POLKIT=OFF`

Dependencies: `polkit`, `glib`

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
Enables windows to grab focus similarly to a context menu under hyprland through the
[hyprland-focus-grab-v1] protocol. This feature has no extra dependencies.

To disable: `-DHYPRLAND_FOCUS_GRAB=OFF`

[hyprland-focus-grab-v1]: https://github.com/hyprwm/hyprland-protocols/blob/main/protocols/hyprland-focus-grab-v1.xml

### i3/Sway
Enables i3 and Sway specific features, does not have any dependency on Wayland or x11.

To disable: `-DI3=OFF`

#### i3/Sway IPC
Enables interfacing with i3 and Sway's IPC.

To disable: `-DI3_IPC=OFF`

## Building
*For developers and prospective contributors: See [CONTRIBUTING.md](CONTRIBUTING.md).*

Only `ninja` builds are tested, but makefiles may work.

#### Configuring the build
```sh
$ cmake -GNinja -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo [additional disable flags from above here]
```

Note that features you do not supply dependencies for MUST be disabled with their associated flags
or quickshell will fail to build.

Additionally, note that clang builds much faster than gcc if you care.

#### Building
```sh
$ cmake --build build
```

#### Installing
```sh
$ cmake --install build
```
