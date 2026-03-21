{
  qtver,
  compiler,
}: let
  checkouts = import ./nix-checkouts.nix;
  nixpkgs = checkouts.${builtins.replaceStrings ["."] ["_"] qtver};
  compilerOverride = (nixpkgs.callPackage ./variations.nix {}).${compiler};
  pkg = (nixpkgs.callPackage ../default.nix {}).override (compilerOverride // {
    wayland-protocols = checkouts.latest.wayland-protocols;
  });
in pkg
