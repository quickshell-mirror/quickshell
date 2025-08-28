{
  inputs = {
    nixpkgs.url = "nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }: let
    forEachSystem = fn:
      nixpkgs.lib.genAttrs
        nixpkgs.lib.platforms.linux
        (system: fn system nixpkgs.legacyPackages.${system});
  in {
    packages = forEachSystem (system: pkgs: rec {
      quickshell-unwrapped = pkgs.callPackage ./unwrapped.nix {
        gitRev = self.rev or self.dirtyRev;
      };
      quickshell = pkgs.callPackage ./default.nix {inherit quickshell-unwrapped;};
      default = quickshell;
    });

    devShells = forEachSystem (system: pkgs: rec {
      default = import ./shell.nix {
        inherit pkgs;
        inherit (self.packages.${system}) quickshell;
      };
    });
  };
}
