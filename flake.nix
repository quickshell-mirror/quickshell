{
  inputs = {
    nixpkgs.url = "nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }: let
    overlayPkgs = p: p.appendOverlays [ self.overlays.default ];

    forEachSystem = fn:
      nixpkgs.lib.genAttrs
        nixpkgs.lib.platforms.linux
        (system: fn system (overlayPkgs nixpkgs.legacyPackages.${system}));
  in {
    overlays.default = import ./overlay.nix {
      rev = self.rev or self.dirtyRev;
    };

    packages = forEachSystem (system: pkgs: rec {
      quickshell = pkgs.quickshell;
      default = quickshell;
    });

    devShells = forEachSystem (system: pkgs: rec {
      default = import ./shell.nix {
        inherit pkgs;
        quickshell = self.packages.${system}.quickshell.override {
          stdenv = pkgs.clangStdenv;
        };
      };
    });
  };
}
