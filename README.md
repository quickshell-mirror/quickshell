# quickshell
<a href="https://matrix.to/#/#quickshell:outfoxxed.me"><img src="https://img.shields.io/badge/Join%20the%20matrix%20room-%23quickshell:outfoxxed.me-0dbd8b?logo=matrix&style=flat-square"></a>

Simple and flexbile QtQuick based desktop shell toolkit.

Hosted on: [outfoxxed's gitea], [github]

[outfoxxed's gitea]: https://git.outfoxxed.me/outfoxxed/quickshell
[github]: https://github.com/outfoxxed/quickshell

Documentation available at [quickshell.outfoxxed.me](https://quickshell.outfoxxed.me) or
can be built from the [quickshell-docs](https://git.outfoxxed.me/outfoxxed/quickshell-docs) repo.

Some fully working examples can be found in the [quickshell-examples](https://git.outfoxxed.me/outfoxxed/quickshell-examples) 
repo.

Both the documentation and examples are included as submodules with revisions that work with the current
version of quickshell.

You can clone everything with
```
$ git clone --recursive https://git.outfoxxed.me/outfoxxed/quickshell.git
```

Or clone missing submodules later with
```
$ git submodule update --init --recursive
```

# Installation

## Nix
This repo has a nix flake you can use to install the package directly:

```nix
{
  inputs = {
    nixpkgs.url = "nixpkgs/nixos-unstable";

    quickshell = {
      url = "git+https://git.outfoxxed.me/outfoxxed/quickshell";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };
}
```

Quickshell's binary is available at `quickshell.packages.<system>.default` to be added to
lists such as `environment.systemPackages` or `home.packages`.

`quickshell.packages.<system>.nvidia` is also available for nvidia users which fixes some
common crashes.

Note: by default this package is built with clang as it is significantly faster.

## Manual

If not using nix, you'll have to build from source.

### Dependencies
To build quickshell at all, you will need the following packages (names may vary by distro)

- just
- cmake
- ninja
- pkg-config
- Qt6 [ QtBase, QtDeclarative ]

Jemalloc is recommended, in which case you will need:
- jemalloc

To build with wayland support you will additionally need:
- wayland
- wayland-scanner (may be part of wayland on some distros)
- wayland-protocols
- Qt6 [ QtWayland ]

To build with x11 support you will additionally need:
- libxcb

To build with pipewire support you will additionally need:
- libpipewire

### Building

To make a release build of quickshell run:
```sh
$ just release
```

If running an nvidia GPU, instead run:
```sh
$ just configure release -DNVIDIA_COMPAT=ON
$ just build
```

(These commands are just aliases for cmake commands you can run directly,
see the Justfile for more information.)

If you have all the dependencies installed and they are in expected
locations this will build correctly.

To install to /usr/local/bin run as root (usually `sudo`) in the same folder:
```
$ just install
```

### Building (Nix)

You can build directly using the provided nix flake or nix package.
```
nix build
nix build -f package.nix # calls default.nix with a basic callPackage expression
```

# Development

For nix there is a devshell available from `shell.nix` and as a devShell
output from the flake.

The Justfile contains various useful aliases:
- `just configure [<debug|release> [extra cmake args]]`
- `just build` (runs configure for debug mode)
- `just run [args]`
- `just clean`
- `just test [args]` (configure with `-DBUILD_TESTING=ON` first)
- `just fmt`
- `just lint`

#### License

<sup>
Licensed under the GNU LGPL 3.
</sup>

<br>

<sub>
Unless you explicitly state otherwise, any contribution submitted
for inclusion shall be licensed as above, without any additional
terms or conditions.
</sub>
