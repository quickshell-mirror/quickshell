self: {
  config,
  pkgs,
  lib,
  ...
}: let
  # ALWAYS acquire the relevant functions from the appropriate parent attribute in lib.
  # if I see even ONE with lib; or `inherit (lib)` in this module SO HELP ME GOD you will
  # be defenestrated using a battle ram and I will piss on your grave
  # - raf
  inherit (lib.modules) mkIf;
  inherit (lib.meta) getExe;
  inherit (lib.types) nullOr listOf package str;
  inherit (lib.options) mkOption mkEnableOption literalExpression;

  defaultPackage = self.packages.${pkgs.stdenv.hostPlatform.system}.default;
  cfg = config.programs.quickshell;
in {
  options.programs.quickshell = {
    enable = mkEnableOption "quickshell";

    package = mkOption {
      type = nullOr package;
      default = defaultPackage;
      defaultText = literalExpression "inputs.quickshell.packages.${pkgs.stdenv.hostPlatform.system}.default";
      description = ''
        The quickshell package to use.

        By default, this option will use the `packages.default` as exposed by this flake.
      '';
    };

    runtimeDependencies = mkOption {
      type = listOf package;
      default = [];
      description = ''
        Additional runtime dependencies for quickshell.
      '';
    };

    systemdTarget = mkOption {
      type = str;
      default = "graphical-session.target";
      example = "sway-session.target";
      description = ''
        Systemd target to bind to.
      '';
    };
  };

  config = mkIf cfg.enable {
    home.packages = [cfg.Package];

    systemd.user.services.quickshell = {
      Install.WantedBy = [cfg.systemdTarget];

      Unit = {
        Description = "quickshell";
        After = ["graphical-session-pre.target"];
        PartOf = [
          "tray.target"
          "graphical-session.target"
        ];
      };

      Service = {
        Type = "simple";
        ExecStart = "${getExe cfg.package}";
        Environment = "PATH=/run/wrappers/bin:${lib.makeBinPath cfg.runtimeDependencies}";
      };
    };
  };
}
