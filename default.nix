{
  lib,
  nix-gitignore,
  pkgs,
  keepDebugInfo,
  buildStdenv ? pkgs.clang17Stdenv,

  cmake,
  ninja,
  qt6,
  jemalloc,
  wayland,
  wayland-protocols,
  xorg,
  pipewire,
  pam,

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
  withJemalloc ? true, # masks heap fragmentation
  withQtSvg ? true,
  withWayland ? true,
  withX11 ? true,
  withPipewire ? true,
  withPam ? true,
  withHyprland ? true,
}: buildStdenv.mkDerivation {
  pname = "quickshell${lib.optionalString debug "-debug"}";
  version = "0.1.0";
  src = nix-gitignore.gitignoreSource "/docs\n/examples\n" ./.;

  nativeBuildInputs = with pkgs; [
    cmake
    ninja
    qt6.wrapQtAppsHook
    pkg-config
  ] ++ (lib.optionals withWayland [
    wayland-protocols
    wayland-scanner
  ]);

  buildInputs = [
    qt6.qtbase
    qt6.qtdeclarative
  ]
  ++ (lib.optional withJemalloc jemalloc)
  ++ (lib.optional withQtSvg qt6.qtsvg)
  ++ (lib.optionals withWayland [ qt6.qtwayland wayland ])
  ++ (lib.optional withX11 xorg.libxcb)
  ++ (lib.optional withPam pam)
  ++ (lib.optional withPipewire pipewire);

  QTWAYLANDSCANNER = lib.optionalString withWayland "${qt6.qtwayland}/libexec/qtwaylandscanner";

  cmakeBuildType = if debug then "Debug" else "RelWithDebInfo";

  cmakeFlags = [ "-DGIT_REVISION=${gitRev}" ]
  ++ lib.optional (!withJemalloc) "-DUSE_JEMALLOC=OFF"
  ++ lib.optional (!withWayland) "-DWAYLAND=OFF"
  ++ lib.optional (!withPipewire) "-DSERVICE_PIPEWIRE=OFF"
  ++ lib.optional (!withPam) "-DSERVICE_PAM=OFF"
  ++ lib.optional (!withHyprland) "-DHYPRLAND=OFF";

  buildPhase = "ninjaBuildPhase";
  enableParallelBuilding = true;

  # How to get debuginfo in gdb from a release build:
  # 1. build `quickshell.debug`
  # 2. set NIX_DEBUG_INFO_DIRS="<quickshell.debug store path>/lib/debug"
  # 3. launch gdb / coredumpctl and debuginfo will work
  separateDebugInfo = !debug;
  dontStrip = debug;

  meta = with lib; {
    homepage = "https://git.outfoxxed.me/outfoxxed/quickshell";
    description = "Simple and flexbile QtQuick based desktop shell toolkit";
    license = licenses.lgpl3Only;
    platforms = platforms.linux;
  };
}
