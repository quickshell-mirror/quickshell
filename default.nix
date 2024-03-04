{
  lib,
  nix-gitignore,
  pkgs,
  keepDebugInfo,
  stdenv ? (keepDebugInfo pkgs.stdenv),

  cmake,
  ninja,
  qt6,
  wayland,
  wayland-protocols,

  debug ? false,
  enableWayland ? true,
}: stdenv.mkDerivation {
  pname = "quickshell${lib.optionalString debug "-debug"}";
  version = "0.1";
  src = nix-gitignore.gitignoreSource [] ./.;

  nativeBuildInputs = with pkgs; [
    cmake
    ninja
    qt6.wrapQtAppsHook
  ] ++ (lib.optionals enableWayland [
    pkg-config
    wayland-protocols
    wayland-scanner
  ]);

  buildInputs = with pkgs; [
    qt6.qtbase
    qt6.qtdeclarative
  ] ++ (lib.optionals enableWayland [ qt6.qtwayland wayland ]);

  QTWAYLANDSCANNER = lib.optionalString enableWayland "${qt6.qtwayland}/libexec/qtwaylandscanner";

  configurePhase = let
    cmakeBuildType = if debug
                     then "Debug"
                     else "RelWithDebInfo";
  in ''
    cmakeBuildType=${cmakeBuildType} # qt6 setup hook resets this for some godforsaken reason
    cmakeConfigurePhase
  '';

  cmakeFlags = lib.optional (!enableWayland) "-DWAYLAND=OFF";

  buildPhase = "ninjaBuildPhase";
  enableParallelBuilding = true;
  dontStrip = true;

  meta = with lib; {
    homepage = "https://git.outfoxxed.me/outfoxxed/quickshell";
    description = "Simple and flexbile QtQuick based desktop shell toolkit";
    platforms = platforms.linux;
  };
}
