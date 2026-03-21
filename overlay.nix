{ rev ? null }: (final: prev: {
  quickshell = final.callPackage ./default.nix {
    gitRev = rev;
  };
})
