{
  qtver,
  compiler,
}: let
  nixpkgs = (import ./nix-checkouts.nix).${builtins.replaceStrings ["."] ["_"] qtver};
  compilerOverride = (nixpkgs.callPackage ./variations.nix {}).${compiler};
  pkg = nixpkgs.callPackage ../default.nix {
    quickshell-unwrapped = (nixpkgs.callPackage ../unwrapped.nix {}).override compilerOverride;
  };
in pkg
