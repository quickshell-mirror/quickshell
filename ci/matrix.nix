{
  qtver,
  compiler,
}: let
  nixpkgs = (import ./nix-checkouts.nix).${builtins.replaceStrings ["."] ["_"] qtver};
  compilerOverride = (nixpkgs.callPackage ./variations.nix {}).${compiler};
  pkg = (nixpkgs.callPackage ../default.nix {}).override compilerOverride;
in pkg
