{
  description = "noctalia-qs - flexible QtQuick based desktop shell toolkit for Noctalia";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    systems.url = "github:nix-systems/default-linux";
    treefmt-nix = {
      url = "github:numtide/treefmt-nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      systems,
      treefmt-nix,
      ...
    }:
    let
      eachSystem =
        fn: nixpkgs.lib.genAttrs (import systems) (system: fn nixpkgs.legacyPackages.${system});

      treefmtEval = eachSystem (pkgs: treefmt-nix.lib.evalModule pkgs ./nix/treefmt.nix);

      mkDate =
        longDate:
        nixpkgs.lib.concatStringsSep "-" [
          (builtins.substring 0 4 longDate)
          (builtins.substring 4 2 longDate)
          (builtins.substring 6 2 longDate)
        ];

      version = mkDate (self.lastModifiedDate or "19700101") + "_" + (self.shortRev or "dirty");
      gitRev = self.rev or self.dirtyRev or "dirty";
    in
    {
      overlays.default = final: _prev: {
        quickshell = final.callPackage ./nix/package.nix { inherit version gitRev; };
      };

      packages = eachSystem (pkgs: {
        quickshell = pkgs.callPackage ./nix/package.nix { inherit version gitRev; };
        default = self.packages.${pkgs.stdenv.hostPlatform.system}.quickshell;
      });

      devShells = eachSystem (pkgs: {
        default = pkgs.callPackage ./nix/shell.nix {
          quickshell = self.packages.${pkgs.stdenv.hostPlatform.system}.quickshell.override {
            stdenv = pkgs.clangStdenv;
          };
        };
      });

      formatter = eachSystem (pkgs: treefmtEval.${pkgs.system}.config.build.wrapper);
    };
}
