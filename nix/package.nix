{
  lib,
  stdenv,
  pkg-config,
  cmake,
  ninja,
  spirv-tools,
  qt6,
  jemalloc,
  cli11,
  wayland,
  wayland-protocols,
  wayland-scanner,
  libxcb,
  libdrm,
  libgbm,
  vulkan-headers,
  pipewire,
  pam,
  glib,
  polkit,
  cpptrace,
  libunwind,
  version,
  gitRev,
  storeDir ? builtins.storeDir,
}:
stdenv.mkDerivation {
  pname = "quickshell";
  inherit version;

  src = lib.fileset.toSource {
    root = ../.;
    fileset = lib.fileset.unions [
      ../src
      ../cmake
      ../assets
      ../CMakeLists.txt
    ];
  };

  nativeBuildInputs = [
    cmake
    ninja
    qt6.qtshadertools
    spirv-tools
    wayland-scanner
    qt6.wrapQtAppsHook
    pkg-config
  ];

  buildInputs = [
    qt6.qtbase
    qt6.qtdeclarative
    qt6.qtwayland
    qt6.qtsvg
    cli11
    wayland
    wayland-protocols
    libdrm
    libgbm
    vulkan-headers
    libxcb
    jemalloc
    pam
    pipewire
    polkit
    glib
    (cpptrace.overrideAttrs (old: {
      cmakeFlags = (old.cmakeFlags or [ ]) ++ [
        (lib.cmakeBool "CPPTRACE_UNWIND_WITH_LIBUNWIND" true)
      ];
      buildInputs = (old.buildInputs or [ ]) ++ [ libunwind ];
    }))
  ];

  cmakeFlags = [
    (lib.cmakeFeature "DISTRIBUTOR" "Official-Nix-Flake")
    (lib.cmakeBool "DISTRIBUTOR_DEBUGINFO_AVAILABLE" true)
    (lib.cmakeBool "CRASH_HANDLER" true)
    (lib.cmakeFeature "INSTALL_QML_PREFIX" qt6.qtbase.qtQmlPrefix)
    (lib.cmakeFeature "GIT_REVISION" gitRev)
    (lib.cmakeBool "NIX_STORE_DIR_SKIP_WATCH" true)
    (lib.cmakeFeature "NIX_STORE_DIR" storeDir)
  ];

  cmakeBuildType = "Release";

  meta = {
    homepage = "https://github.com/noctalia-dev/noctalia-qs";
    description = "Flexible QtQuick based desktop shell toolkit for Noctalia";
    license = lib.licenses.lgpl3Only;
    platforms = lib.platforms.linux;
    mainProgram = "quickshell";
  };
}
