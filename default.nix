{
  lib,
  nix-gitignore,
  pkgs,
  stdenv,
  keepDebugInfo,

  pkg-config,
  cmake,
  ninja,
  spirv-tools,
  qt6,
  breakpad,
  jemalloc,
  cli11,
  wayland,
  wayland-protocols,
  wayland-scanner,
  xorg,
  libdrm,
  libgbm ? null,
  vulkan-headers,
  pipewire,
  pam,
  polkit,
  glib,

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
  withCrashReporter ? true,
  withJemalloc ? true, # masks heap fragmentation
  withQtSvg ? true,
  withWayland ? true,
  withX11 ? true,
  withPipewire ? true,
  withPam ? true,
  withHyprland ? true,
  withI3 ? true,
  withPolkit ? true,
  withNetworkManager ? true,
}: let
  unwrapped = stdenv.mkDerivation {
    pname = "quickshell${lib.optionalString debug "-debug"}";
    version = "0.2.1";
    src = nix-gitignore.gitignoreSource "/default.nix\n" ./.;

    dontWrapQtApps = true; # see wrappers

    nativeBuildInputs = [
      cmake
      ninja
      spirv-tools
      pkg-config
    ]
    ++ lib.optional (withWayland && lib.strings.compareVersions qt6.qtbase.version "6.10.0" == -1) qt6.qtwayland
    ++ lib.optionals withWayland [
      qt6.qtwayland # qtwaylandscanner required at build time
      wayland-scanner
    ];

    buildInputs = [
      qt6.qtbase
      qt6.qtdeclarative
      cli11
    ]
    ++ lib.optional withQtSvg qt6.qtsvg
    ++ lib.optional withCrashReporter breakpad
    ++ lib.optional withJemalloc jemalloc
    ++ lib.optional (withWayland && lib.strings.compareVersions qt6.qtbase.version "6.10.0" == -1) qt6.qtwayland
    ++ lib.optionals withWayland [ wayland wayland-protocols ]
    ++ lib.optionals (withWayland && libgbm != null) [ libdrm libgbm vulkan-headers ]
    ++ lib.optional withX11 xorg.libxcb
    ++ lib.optional withPam pam
    ++ lib.optional withPipewire pipewire
    ++ lib.optionals withPolkit [ polkit glib ];

    cmakeBuildType = if debug then "Debug" else "RelWithDebInfo";

    cmakeFlags = [
      (lib.cmakeFeature "DISTRIBUTOR" "Official-Nix-Flake")
      (lib.cmakeFeature "INSTALL_QML_PREFIX" qt6.qtbase.qtQmlPrefix)
      (lib.cmakeBool "DISTRIBUTOR_DEBUGINFO_AVAILABLE" true)
      (lib.cmakeFeature "GIT_REVISION" gitRev)
      (lib.cmakeBool "CRASH_REPORTER" withCrashReporter)
      (lib.cmakeBool "USE_JEMALLOC" withJemalloc)
      (lib.cmakeBool "WAYLAND" withWayland)
      (lib.cmakeBool "SCREENCOPY" (libgbm != null))
      (lib.cmakeBool "SERVICE_PIPEWIRE" withPipewire)
      (lib.cmakeBool "SERVICE_PAM" withPam)
      (lib.cmakeBool "SERVICE_NETWORKMANAGER" withNetworkManager)
      (lib.cmakeBool "SERVICE_POLKIT" withPolkit)
      (lib.cmakeBool "HYPRLAND" withHyprland)
      (lib.cmakeBool "I3" withI3)
    ];

    # How to get debuginfo in gdb from a release build:
    # 1. build `quickshell.debug`
    # 2. set NIX_DEBUG_INFO_DIRS="<quickshell.debug store path>/lib/debug"
    # 3. launch gdb / coredumpctl and debuginfo will work
    separateDebugInfo = !debug;
    dontStrip = debug;

    meta = with lib; {
      homepage = "https://quickshell.org";
      description = "Flexbile QtQuick based desktop shell toolkit";
      license = licenses.lgpl3Only;
      platforms = platforms.linux;
      mainProgram = "quickshell";
    };
  };

  wrapper = unwrapped.stdenv.mkDerivation {
    inherit (unwrapped) version meta buildInputs;
    pname = "${unwrapped.pname}-wrapped";

    nativeBuildInputs = unwrapped.nativeBuildInputs ++ [ qt6.wrapQtAppsHook ];

    dontUnpack = true;
    dontConfigure = true;
    dontBuild = true;

    installPhase = ''
      mkdir -p $out
      cp -r ${unwrapped}/* $out
    '';

    passthru = {
      unwrapped = unwrapped;
      withModules = modules: wrapper.overrideAttrs (prev: {
        buildInputs = prev.buildInputs ++ modules;
      });
    };
  };
in wrapper
