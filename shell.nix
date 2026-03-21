{
  pkgs ? import <nixpkgs> {},
  stdenv ? pkgs.clangStdenv, # faster compiles than gcc
  quickshell ? pkgs.callPackage ./default.nix { inherit stdenv; },
  ...
}: let
  tidyfox = import (pkgs.fetchFromGitea {
    domain = "git.outfoxxed.me";
    owner = "outfoxxed";
    repo = "tidyfox";
    rev = "9d85d7e7dea2602aa74ec3168955fee69967e92f";
    hash = "sha256-77ERiweF6lumonp2c/124rAoVG6/o9J+Aajhttwtu0w=";
  }) { inherit pkgs; };
in pkgs.mkShell.override { stdenv = quickshell.stdenv; } {
  inputsFrom = [ quickshell ];

  nativeBuildInputs = with pkgs; [
    just
    clang-tools
    parallel
    makeWrapper
  ];

  TIDYFOX = "${tidyfox}/lib/libtidyfox.so";

  shellHook = ''
    export CMAKE_BUILD_PARALLEL_LEVEL=$(nproc)

    # Add Qt-related environment variables.
    # https://discourse.nixos.org/t/qt-development-environment-on-a-flake-system/23707/5
    setQtEnvironment=$(mktemp)
    random=$(openssl rand -base64 20 | sed "s/[^a-zA-Z0-9]//g")
    makeShellWrapper "$(type -p sh)" "$setQtEnvironment" "''${qtWrapperArgs[@]}" --argv0 "$random"
    sed "/$random/d" -i "$setQtEnvironment"
    source "$setQtEnvironment"

    # qmlls does not account for the import path and bases its search off qtbase's path.
    # The actual imports come from qtdeclarative. This directs qmlls to the correct imports.
    export QMLLS_BUILD_DIRS=$(pwd)/build:$QML2_IMPORT_PATH
  '';
}
