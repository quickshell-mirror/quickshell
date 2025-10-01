{
  callPackage,
  quickshell-unwrapped ? callPackage ./unwrapped.nix {},
  qt6,
}: quickshell-unwrapped.stdenv.mkDerivation (final: {
  inherit (quickshell-unwrapped) version meta buildInputs;
  pname = "${quickshell-unwrapped.pname}-wrapped";

  nativeBuildInputs = quickshell-unwrapped.nativeBuildInputs ++ [ qt6.wrapQtAppsHook ];

  dontUnpack = true;
  dontConfigure = true;
  dontBuild = true;

  installPhase = ''
    mkdir -p $out
    cp -r ${quickshell-unwrapped}/* $out
  '';

  passthru = {
    unwrapped = quickshell-unwrapped;
    withModules = modules: final.finalPackage.overrideAttrs (prev: {
      buildInputs = prev.buildInputs ++ modules;
    });
  };
})
