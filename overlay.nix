{ rev ? null }: (final: prev: {
  quickshell-unwrapped = final.callPackage ./unwrapped.nix { gitRev = rev; };
  quickshell = final.callPackage ./default.nix {};
})
