{
  callPackage,
  lib,
  quickshell-unwrapped ? callPackage ./unwrapped.nix {},
  qt6,
}: let
  unwrapped = quickshell-unwrapped;
  inherit (unwrapped.stdenv) mkDerivation;
  inherit (lib) typeOf asserts;
in
  mkDerivation (final: {
    inherit (unwrapped) version meta buildInputs;
    pname = "${unwrapped.pname}-wrapped";

    nativeBuildInputs = unwrapped.nativeBuildInputs ++ [qt6.wrapQtAppsHook];

    dontUnpack = true;
    dontConfigure = true;
    dontBuild = true;

    installPhase = ''
      mkdir -p $out
      cp -r ${unwrapped}/* $out
    '';

    passthru = {
      inherit unwrapped;
      withModules = modules:
        assert (asserts.assertMsg (typeOf modules == "list") "`modules` must be a list of packages");
          final.finalPackage.overrideAttrs (prev: {
            buildInputs = prev.buildInputs ++ modules;
          });
    };
  })
