{
  qtver,
  compiler,
}: let
  checkouts = import ./nix-checkouts.nix;
  nixpkgs = checkouts.${builtins.replaceStrings ["."] ["_"] qtver};
  compilerOverride = (nixpkgs.callPackage ./variations.nix {}).${compiler};
  quickshell-unwrapped = nixpkgs.callPackage ../unwrapped.nix (
    compilerOverride //
    {inherit (checkouts.latest) wayland-protocols;}
  );
  pkg = (nixpkgs.callPackage ../default.nix {inherit quickshell-unwrapped;});
in pkg
