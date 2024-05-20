{
  lib,
  nix-gitignore,
  pkgs,
  keepDebugInfo,
  buildStdenv ? pkgs.clang17Stdenv,

  cmake,
  ninja,
  qt6,
  wayland,
  wayland-protocols,
  xorg,
  pipewire,

  gitRev ? (let
    headExists = builtins.pathExists ./.git/HEAD;
    headContent = builtins.readFile ./.git/HEAD;
  in if headExists
     then (let
       matches = builtins.match "ref: refs/heads/(.*)\n" headContent;
     in if matches != null
        then builtins.readFile ./.git/refs/heads/${builtins.elemAt matches 0}
        else headContent)
     else "unknown"),

  debug ? false,
  enableWayland ? true,
  enableX11 ? true,
  enablePipewire ? true,
  nvidiaCompat ? false,
  svgSupport ? true, # you almost always want this
}: buildStdenv.mkDerivation {
  pname = "quickshell${lib.optionalString debug "-debug"}";
  version = "0.1.0";
  src = nix-gitignore.gitignoreSource "/docs\n/examples\n" ./.;

  nativeBuildInputs = with pkgs; [
    cmake
    ninja
    qt6.wrapQtAppsHook
  ] ++ (lib.optionals enableWayland [
    pkg-config
    wayland-protocols
    wayland-scanner
  ]);

  buildInputs = [
    qt6.qtbase
    qt6.qtdeclarative
  ]
  ++ (lib.optionals enableWayland [ qt6.qtwayland wayland ])
  ++ (lib.optionals enableX11 [ xorg.libxcb ])
  ++ (lib.optionals svgSupport [ qt6.qtsvg ])
  ++ (lib.optionals enablePipewire [ pipewire ]);

  QTWAYLANDSCANNER = lib.optionalString enableWayland "${qt6.qtwayland}/libexec/qtwaylandscanner";

  configurePhase = let
    cmakeBuildType = if debug
                     then "Debug"
                     else "RelWithDebInfo";
  in ''
    cmakeBuildType=${cmakeBuildType} # qt6 setup hook resets this for some godforsaken reason
    cmakeConfigurePhase
  '';

  cmakeFlags = [
    "-DGIT_REVISION=${gitRev}"
  ] ++ lib.optional (!enableWayland) "-DWAYLAND=OFF"
  ++ lib.optional nvidiaCompat "-DNVIDIA_COMPAT=ON"
  ++ lib.optional (!enablePipewire) "-DSERVICE_PIPEWIRE=OFF";

  buildPhase = "ninjaBuildPhase";
  enableParallelBuilding = true;
  dontStrip = true;

  meta = with lib; {
    homepage = "https://git.outfoxxed.me/outfoxxed/quickshell";
    description = "Simple and flexbile QtQuick based desktop shell toolkit";
    license = licenses.lgpl3Only;
    platforms = platforms.linux;
  };
}
