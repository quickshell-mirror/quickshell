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

The package contains several features detailed in [BUILD.md](BUILD.md) which can be enabled
or disabled with overrides:

```nix
quickshell.packages.<system>.default.override {
  enableWayland = true;
  enableX11 = true;
  enablePipewire = true;
  withQtSvg = true;
  withJemalloc = true;
}
```

Note: by default this package is built with clang as it is significantly faster.

## Arch (AUR)
Quickshell has a third party [AUR package] available under the same name.
As is usual with the AUR it is not maintained by us and should be looked over before use.

[AUR package]: https://aur.archlinux.org/packages/quickshell

## Anything else
See [BUILD.md](BUILD.md) for instructions on building and packaging quickshell.

# Contributing / Development
See [CONTRIBUTING.md](CONTRIBUTING.md) for details.

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
