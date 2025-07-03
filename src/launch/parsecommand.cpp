#include <cstddef>
#include <limits>
#include <memory>

#include <CLI/App.hpp>
#include <CLI/CLI.hpp> // NOLINT: Need to include this for impls of some CLI11 classes
#include <CLI/Validators.hpp>

#include "launch_p.hpp"

namespace qs::launch {

int parseCommand(int argc, char** argv, CommandState& state) {
	state.exec = {
	    .argc = argc,
	    .argv = argv,
	};

	auto addConfigSelection = [&](CLI::App* cmd, bool withNewestOption = false) {
		auto* group =
		    cmd->add_option_group("Config Selection")
		        ->description(
		            "Quickshell detects configurations as named directories under each XDG config "
		            "directory as `<xdg dir>/quickshell/<config name>/shell.qml`.\n\n"
		            "If `<xdg dir>/quickshell/shell.qml` exists, it will be registered as the "
		            "'default' configuration, and no subdirectories will be considered. "
		            "If --config is not passed, 'default' will be assumed.\n\n"
		            "Alternatively, a config can be selected by path with --path.\n\n"
		            "Examples:\n"
		            "- `~/.config/quickshell/shell.qml` can be run with `qs`\n"
		            "- `/etc/xdg/quickshell/myconfig/shell.qml` can be run with `qs -c myconfig`\n"
		            "- `~/myshell/shell.qml` can be run with `qs -p ~/myshell`\n"
		            "- `~/myshell/randomfile.qml` can be run with `qs -p ~/myshell/randomfile.qml`"
		        );

		auto* path = group->add_option("-p,--path", state.config.path)
		                 ->description("Path to a QML file or config folder.")
		                 ->envname("QS_CONFIG_PATH");

		group->add_option("-c,--config", state.config.name)
		    ->description("Name of a quickshell configuration to run.")
		    ->envname("QS_CONFIG_NAME")
		    ->excludes(path);

		group->add_option("-m,--manifest", state.config.manifest)
		    ->description("[DEPRECATED] Path to a quickshell manifest.\n"
		                  "If a manifest is specified, configs named by -c will point to its entries.\n"
		                  "Defaults to $XDG_CONFIG_HOME/quickshell/manifest.conf")
		    ->envname("QS_MANIFEST")
		    ->excludes(path);

		if (withNewestOption) {
			group->add_flag("-n,--newest", state.config.newest)
			    ->description("Operate on the most recently launched instance instead of the oldest");
		}

		return group;
	};

	auto addDebugOptions = [&](CLI::App* cmd) {
		auto* group = cmd->add_option_group("Debugging", "Options for QML debugging.");

		auto* debug = group->add_option("--debug", state.debug.port)
		                  ->description("Open the given port for a QML debugger connection.")
		                  ->check(CLI::Range(0, 65535));

		group->add_flag("--waitfordebug", state.debug.wait)
		    ->description("Wait for a QML debugger to connect before executing the configuration.")
		    ->needs(debug);

		return group;
	};

	auto addLoggingOptions = [&](CLI::App* cmd, bool noGroup, bool noDisplay = false) {
		auto* group = noGroup ? cmd : cmd->add_option_group(noDisplay ? "" : "Logging");

		group->add_flag("--no-color", state.log.noColor)
		    ->description("Disables colored logging.\n"
		                  "Colored logging can also be disabled by specifying a non empty value "
		                  "for the NO_COLOR environment variable.");

		group->add_flag("--log-times", state.log.timestamp)
		    ->description("Log timestamps with each message.");

		group->add_option("--log-rules", state.log.rules)
		    ->description("Log rules to apply, in the format of QT_LOGGING_RULES.");

		group->add_flag("-v,--verbose", [&](size_t count) { state.log.verbosity = count; })
		    ->description("Increases log verbosity.\n"
		                  "-v will show INFO level internal logs.\n"
		                  "-vv will show DEBUG level internal logs.");

		auto* hgroup = cmd->add_option_group("");
		hgroup->add_flag("--no-detailed-logs", state.log.sparse);
	};

	auto addInstanceSelection = [&](CLI::App* cmd) {
		auto* group = cmd->add_option_group("Instance Selection");

		group->add_option("-i,--id", state.instance.id)
		    ->description("The instance id to operate on.\n"
		                  "You may also use a substring the id as long as it is unique, "
		                  "for example \"abc\" will select \"abcdefg\".");

		group->add_option("--pid", state.instance.pid)
		    ->description("The process id of the instance to operate on.");

		return group;
	};

	state.app = std::make_unique<CLI::App>();
	auto* cli = state.app.get();

	// Require 0-1 subcommands. Without this, positionals can be parsed as more subcommands.
	cli->require_subcommand(0, 1);

	addConfigSelection(cli);
	addLoggingOptions(cli, false);
	addDebugOptions(cli);

	{
		cli->add_option_group("")->add_flag("--private-check-compat", state.misc.checkCompat);

		cli->add_flag("-V,--version", state.misc.printVersion)
		    ->description("Print quickshell's version and exit.");

		cli->add_flag("-n,--no-duplicate", state.misc.noDuplicate)
		    ->description("Exit immediately if another instance of the given config is running.");

		cli->add_flag("-d,--daemonize", state.misc.daemonize)
		    ->description("Detach from the controlling terminal.");
	}

	{
		auto* sub = cli->add_subcommand("log", "Print quickshell logs.");

		auto* file = sub->add_option("file", state.log.file, "Log file to read.");

		sub->add_option("-t,--tail", state.log.tail)
		    ->description("Maximum number of lines to print, starting from the bottom.")
		    ->check(CLI::Range(1, std::numeric_limits<int>::max(), "INT > 0"));

		sub->add_flag("-f,--follow", state.log.follow)
		    ->description("Keep reading the log until the logging process terminates.");

		sub->add_option("-r,--rules", state.log.readoutRules, "Log file to read.")
		    ->description("Rules to apply to the log being read, in the format of QT_LOGGING_RULES.");

		auto* instance = addInstanceSelection(sub)->excludes(file);
		addConfigSelection(sub, true)->excludes(instance)->excludes(file);
		addLoggingOptions(sub, false);

		state.subcommand.log = sub;
	}

	{
		auto* sub = cli->add_subcommand("list", "List running quickshell instances.");

		auto* all = sub->add_flag("-a,--all", state.instance.all)
		                ->description("List all instances.\n"
		                              "If unspecified, only instances of"
		                              "the selected config will be listed.");

		sub->add_flag("-j,--json", state.output.json, "Output the list as a json.");

		sub->add_flag("--show-dead", state.instance.includeDead)
		    ->description("Include dead instances in the list.");

		addConfigSelection(sub, true)->excludes(all);
		addLoggingOptions(sub, false, true);

		state.subcommand.list = sub;
	}

	{
		auto* sub = cli->add_subcommand("kill", "Kill quickshell instances.");
		//sub->add_flag("-a,--all", "Kill all matching instances instead of just one.");
		auto* instance = addInstanceSelection(sub);
		addConfigSelection(sub, true)->excludes(instance);
		addLoggingOptions(sub, false, true);

		state.subcommand.kill = sub;
	}

	{
		auto* sub = cli->add_subcommand("ipc", "Communicate with other Quickshell instances.")
		                ->require_subcommand();
		state.ipc.ipc = sub;

		auto* instance = addInstanceSelection(sub);
		addConfigSelection(sub, true)->excludes(instance);
		addLoggingOptions(sub, false, true);

		{
			auto* show = sub->add_subcommand("show", "Print information about available IPC targets.");
			state.ipc.show = show;
		}

		{
			auto* call = sub->add_subcommand("call", "Call an IpcHandler function.");
			state.ipc.call = call;

			call->add_option("target", state.ipc.target, "The target to message.");

			call->add_option("function", state.ipc.name)
			    ->description("The function to call in the target.");

			call->add_option("arguments", state.ipc.arguments)
			    ->description("Arguments to the called function.")
			    ->allow_extra_args();
		}

		{
			auto* prop =
			    sub->add_subcommand("prop", "Manipulate IpcHandler properties.")->require_subcommand();

			{
				auto* get = prop->add_subcommand("get", "Read the value of a property.");
				state.ipc.getprop = get;
				get->add_option("target", state.ipc.target, "The target to read the property of.");
				get->add_option("property", state.ipc.name)->description("The property to read.");
			}
		}
	}

	{
		auto* sub = cli->add_subcommand("msg", "[DEPRECATED] Moved to `ipc call`.")->require_option();

		sub->add_option("target", state.ipc.target, "The target to message.");

		sub->add_option("function", state.ipc.name)->description("The function to call in the target.");

		sub->add_option("arguments", state.ipc.arguments)
		    ->description("Arguments to the called function.")
		    ->allow_extra_args();

		sub->add_flag("-s,--show", state.ipc.showOld)
		    ->description("Print information about a function or target if given, or all available "
		                  "targets if not.");

		auto* instance = addInstanceSelection(sub);
		addConfigSelection(sub, true)->excludes(instance);
		addLoggingOptions(sub, false, true);

		state.subcommand.msg = sub;
	}

	CLI11_PARSE(*cli, argc, argv);

	return 65535;
}

} // namespace qs::launch
