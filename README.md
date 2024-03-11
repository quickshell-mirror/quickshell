# quickshell

Simple and flexbile QtQuick based desktop shell toolkit.

Hosts: [outfoxxed's gitea], [github]

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

## Manual

If not using nix, you'll have to build from source.

### Dependencies
To build quickshell at all, you will need the following packages (names may vary by distro)

- just
- cmake
- pkg-config
- ninja
- Qt6 [ QtBase, QtDeclarative ]

To build with wayland support you will additionally need:
- wayland
- wayland-scanner (may be part of wayland on some distros)
- wayland-protocols
- Qt6 [ QtWayland ]

### Building

To make a release build of quickshell run:
```sh
$ just release
```

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
