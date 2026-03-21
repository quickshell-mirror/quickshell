{
  projectRootFile = "flake.nix";

  # nix
  programs.nixfmt.enable = true; # formatter
  programs.deadnix.enable = true; # linter
  programs.statix.enable = true; # linter
}
