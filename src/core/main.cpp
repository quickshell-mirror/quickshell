#include "main.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

#include <CLI/App.hpp>
#include <CLI/CLI.hpp> // NOLINT: Need to include this for impls of some CLI11 classes
#include <CLI/Validators.hpp>
#include <fcntl.h>
#include <qapplication.h>
#include <qcontainerfwd.h>
#include <qcoreapplication.h>
#include <qcryptographichash.h>
#include <qdatastream.h>
#include <qdatetime.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qguiapplication.h>
#include <qhash.h>
#include <qicon.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qqmldebug.h>
#include <qquickwindow.h>
#include <qstandardpaths.h>
#include <qstring.h>
#include <qtenvironmentvariables.h>
#include <qtextstream.h>
#include <unistd.h>

#include "../io/ipccomm.hpp"
#include "build.hpp"
#include "common.hpp"
#include "instanceinfo.hpp"
#include "ipc.hpp"
#include "logging.hpp"
#include "paths.hpp"
#include "plugin.hpp"
#include "rootwrapper.hpp"

#if CRASH_REPORTER
#include "../crash/handler.hpp"
#include "../crash/main.hpp"
#endif

namespace qs::launch {

using qs::ipc::IpcClient;

void checkCrashRelaunch(char** argv, QCoreApplication* coreApplication);
int runCommand(int argc, char** argv, QCoreApplication* coreApplication);

int DAEMON_PIPE = -1; // NOLINT
void exitDaemon(int code) {
	if (DAEMON_PIPE == -1) return;

	if (write(DAEMON_PIPE, &code, sizeof(int)) == -1) {
		qCritical().nospace() << "Failed to write daemon exit command with error code " << errno << ": "
		                      << qt_error_string();
	}

	close(DAEMON_PIPE);

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	if (open("/dev/null", O_RDONLY) != STDIN_FILENO) { // NOLINT
		qFatal() << "Failed to open /dev/null on stdin";
	}

	if (open("/dev/null", O_WRONLY) != STDOUT_FILENO) { // NOLINT
		qFatal() << "Failed to open /dev/null on stdout";
	}

	if (open("/dev/null", O_WRONLY) != STDERR_FILENO) { // NOLINT
		qFatal() << "Failed to open /dev/null on stderr";
	}
}

int main(int argc, char** argv) {
	QCoreApplication::setApplicationName("quickshell");

#if CRASH_REPORTER
	qsCheckCrash(argc, argv);
#endif

	auto qArgC = 1;
	auto* coreApplication = new QCoreApplication(qArgC, argv);

	checkCrashRelaunch(argv, coreApplication);
	auto code = runCommand(argc, argv, coreApplication);

	exitDaemon(code);
	return code;
}

class QStringOption {
public:
	QStringOption() = default;
	QStringOption& operator=(const std::string& str) {
		this->str = QString::fromStdString(str);
		return *this;
	}

	QString& operator*() { return this->str; }
	QString* operator->() { return &this->str; }

private:
	QString str;
};

struct CommandState {
	struct {
		int argc = 0;
		char** argv = nullptr;
	} exec;

	struct {
		bool timestamp = false;
		bool noColor = !qEnvironmentVariableIsEmpty("NO_COLOR");
		bool sparse = false;
		size_t verbosity = 0;
		int tail = 0;
		bool follow = false;
		QStringOption rules;
		QStringOption readoutRules;
		QStringOption file;
	} log;

	struct {
		QStringOption path;
		QStringOption manifest;
		QStringOption name;
	} config;

	struct {
		int port = -1;
		bool wait = false;
	} debug;

	struct {
		QStringOption id;
		pid_t pid = -1; // NOLINT (include)
		bool all = false;
	} instance;

	struct {
		bool json = false;
	} output;

	struct {
		bool info = false;
		QStringOption target;
		QStringOption function;
		std::vector<QStringOption> arguments;
	} ipc;

	struct {
		CLI::App* log = nullptr;
		CLI::App* list = nullptr;
		CLI::App* kill = nullptr;
		CLI::App* msg = nullptr;
	} subcommand;

	struct {
		bool printVersion = false;
		bool killAll = false;
		bool noDuplicate = false;
		bool daemonize = false;
	} misc;
};

int readLogFile(CommandState& cmd);
int listInstances(CommandState& cmd);
int killInstances(CommandState& cmd);
int msgInstance(CommandState& cmd);
int launchFromCommand(CommandState& cmd, QCoreApplication* coreApplication);

struct LaunchArgs {
	QString configPath;
	int debugPort = -1;
	bool waitForDebug = false;
};

int launch(const LaunchArgs& args, char** argv, QCoreApplication* coreApplication);

int runCommand(int argc, char** argv, QCoreApplication* coreApplication) {
	auto state = CommandState();

	state.exec = {
	    .argc = argc,
	    .argv = argv,
	};

	auto addConfigSelection = [&](CLI::App* cmd) {
		auto* group = cmd->add_option_group("Config Selection")
		                  ->description("If no options in this group are specified,\n"
		                                "$XDG_CONFIG_HOME/quickshell/shell.qml will be used.");

		auto* path = group->add_option("-p,--path", state.config.path)
		                 ->description("Path to a QML file.")
		                 ->envname("QS_CONFIG_PATH");

		group->add_option("-m,--manifest", state.config.manifest)
		    ->description("Path to a quickshell manifest.\n"
		                  "Defaults to $XDG_CONFIG_HOME/quickshell/manifest.conf")
		    ->envname("QS_MANIFEST")
		    ->excludes(path);

		group->add_option("-c,--config", state.config.name)
		    ->description("Name of a quickshell configuration to run.\n"
		                  "If -m is specified, this is a configuration in the manifest,\n"
		                  "otherwise it is the name of a folder in $XDG_CONFIG_HOME/quickshell.")
		    ->envname("QS_CONFIG_NAME");

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
		                  "Colored logging can also be disabled by specifying a non empty value\n"
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
		                  "You may also use a substring the id as long as it is unique,\n"
		                  "for example \"abc\" will select \"abcdefg\".");

		group->add_option("--pid", state.instance.pid)
		    ->description("The process id of the instance to operate on.");

		return group;
	};

	auto cli = CLI::App();

	// Require 0-1 subcommands. Without this, positionals can be parsed as more subcommands.
	cli.require_subcommand(0, 1);

	addConfigSelection(&cli);
	addLoggingOptions(&cli, false);
	addDebugOptions(&cli);

	{
		cli.add_flag("-V,--version", state.misc.printVersion)
		    ->description("Print quickshell's version and exit.");

		cli.add_flag("-n,--no-duplicate", state.misc.noDuplicate)
		    ->description("Exit immediately if another instance of the given config is running.");

		cli.add_flag("-d,--daemonize", state.misc.daemonize)
		    ->description("Detach from the controlling terminal.");
	}

	{
		auto* sub = cli.add_subcommand("log", "Print quickshell logs.");

		auto* file = sub->add_option("file", state.log.file, "Log file to read.");

		sub->add_option("-t,--tail", state.log.tail)
		    ->description("Maximum number of lines to print, starting from the bottom.")
		    ->check(CLI::Range(1, std::numeric_limits<int>::max(), "INT > 0"));

		sub->add_flag("-f,--follow", state.log.follow)
		    ->description("Keep reading the log until the logging process terminates.");

		sub->add_option("-r,--rules", state.log.readoutRules, "Log file to read.")
		    ->description("Rules to apply to the log being read, in the format of QT_LOGGING_RULES.");

		auto* instance = addInstanceSelection(sub)->excludes(file);
		addConfigSelection(sub)->excludes(instance)->excludes(file);
		addLoggingOptions(sub, false);

		state.subcommand.log = sub;
	}

	{
		auto* sub = cli.add_subcommand("list", "List running quickshell instances.");

		auto* all = sub->add_flag("-a,--all", state.instance.all)
		                ->description("List all instances.\n"
		                              "If unspecified, only instances of"
		                              "the selected config will be listed.");

		sub->add_flag("-j,--json", state.output.json, "Output the list as a json.");

		addConfigSelection(sub)->excludes(all);
		addLoggingOptions(sub, false, true);

		state.subcommand.list = sub;
	}

	{
		auto* sub = cli.add_subcommand("kill", "Kill quickshell instances.");
		//sub->add_flag("-a,--all", "Kill all matching instances instead of just one.");
		auto* instance = addInstanceSelection(sub);
		addConfigSelection(sub)->excludes(instance);
		addLoggingOptions(sub, false, true);

		state.subcommand.kill = sub;
	}

	{
		auto* sub = cli.add_subcommand("msg", "Send messages to IpcHandlers.")->require_option();

		auto* target = sub->add_option("target", state.ipc.target, "The target to message.");

		auto* function = sub->add_option("function", state.ipc.function)
		                     ->description("The function to call in the target.")
		                     ->needs(target);

		auto* arguments = sub->add_option("arguments", state.ipc.arguments)
		                      ->description("Arguments to the called function.")
		                      ->needs(function)
		                      ->allow_extra_args();

		sub->add_flag("-s,--show", state.ipc.info)
		    ->description("Print information about a function or target if given, or all available "
		                  "targets if not.")
		    ->excludes(arguments);

		auto* instance = addInstanceSelection(sub);
		addConfigSelection(sub)->excludes(instance);
		addLoggingOptions(sub, false, true);

		sub->require_option();

		state.subcommand.msg = sub;
	}

	CLI11_PARSE(cli, argc, argv);

	// Has to happen before extra threads are spawned.
	if (state.misc.daemonize) {
		auto closepipes = std::array<int, 2>();
		if (pipe(closepipes.data()) == -1) {
			qFatal().nospace() << "Failed to create messaging pipes for daemon with error " << errno
			                   << ": " << qt_error_string();
		}

		DAEMON_PIPE = closepipes[1];

		pid_t pid = fork(); // NOLINT (include)

		if (pid == -1) {
			qFatal().nospace() << "Failed to fork daemon with error " << errno << ": "
			                   << qt_error_string();
		} else if (pid == 0) {
			close(closepipes[0]);

			if (setsid() == -1) {
				qFatal().nospace() << "Failed to setsid with error " << errno << ": " << qt_error_string();
			}
		} else {
			close(closepipes[1]);

			int ret = 0;
			if (read(closepipes[0], &ret, sizeof(int)) == -1) {
				qFatal() << "Failed to wait for daemon launch (it may have crashed)";
			}

			return ret;
		}
	}

	{
		auto level = state.log.verbosity == 0 ? QtWarningMsg
		           : state.log.verbosity == 1 ? QtInfoMsg
		                                      : QtDebugMsg;

		LogManager::init(
		    !state.log.noColor,
		    state.log.timestamp,
		    state.log.sparse,
		    level,
		    *state.log.rules,
		    *state.subcommand.log ? "READER" : ""
		);
	}

	if (state.misc.printVersion) {
		qCInfo(logBare).noquote() << "quickshell pre-release, revision" << GIT_REVISION;
	} else if (*state.subcommand.log) {
		return readLogFile(state);
	} else if (*state.subcommand.list) {
		return listInstances(state);
	} else if (*state.subcommand.kill) {
		return killInstances(state);
	} else if (*state.subcommand.msg) {
		return msgInstance(state);
	} else {
		return launchFromCommand(state, coreApplication);
	}

	return 0;
}

int locateConfigFile(CommandState& cmd, QString& path) {
	if (!cmd.config.path->isEmpty()) {
		path = *cmd.config.path;
	} else {
		auto manifestPath = *cmd.config.manifest;
		if (manifestPath.isEmpty()) {
			auto configDir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
			auto path = configDir.filePath("manifest.conf");
			if (QFileInfo(path).isFile()) manifestPath = path;
		}

		if (!manifestPath.isEmpty()) {
			auto file = QFile(manifestPath);
			if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
				auto stream = QTextStream(&file);
				while (!stream.atEnd()) {
					auto line = stream.readLine();
					if (line.trimmed().startsWith("#")) continue;
					if (line.trimmed().isEmpty()) continue;

					auto split = line.split('=');
					if (split.length() != 2) {
						qCritical() << "Manifest line not in expected format 'name = relativepath':" << line;
						return -1;
					}

					if (split[0].trimmed() == *cmd.config.name) {
						path = QDir(QFileInfo(file).canonicalPath()).filePath(split[1].trimmed());
						break;
					}
				}

				if (path.isEmpty()) {
					qCCritical(logBare) << "Configuration" << *cmd.config.name
					                    << "not found when searching manifest" << manifestPath;
					return -1;
				}
			} else {
				qCCritical(logBare) << "Could not open maifest at path" << *cmd.config.manifest;
				return -1;
			}
		} else {
			auto configDir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));

			if (cmd.config.name->isEmpty()) {
				path = configDir.path();
			} else {
				path = configDir.filePath(*cmd.config.name);
			}
		}
	}

	if (QFileInfo(path).isDir()) {
		path = QDir(path).filePath("shell.qml");
	}

	if (!QFileInfo(path).isFile()) {
		qCCritical(logBare) << "Could not open config file at" << path;
		return -1;
	}

	path = QFileInfo(path).canonicalFilePath();

	return 0;
}

void sortInstances(QVector<InstanceLockInfo>& list) {
	std::sort(list.begin(), list.end(), [](const InstanceLockInfo& a, const InstanceLockInfo& b) {
		return a.instance.launchTime < b.instance.launchTime;
	});
};

int selectInstance(CommandState& cmd, InstanceLockInfo* instance) {
	auto* basePath = QsPaths::instance()->baseRunDir();
	if (!basePath) return -1;

	QString path;

	if (cmd.instance.pid != -1) {
		path = QDir(basePath->filePath("by-pid")).filePath(QString::number(cmd.instance.pid));
		if (!QsPaths::checkLock(path, instance)) {
			qCInfo(logBare) << "No instance found for pid" << cmd.instance.pid;
			return -1;
		}
	} else if (!cmd.instance.id->isEmpty()) {
		path = basePath->filePath("by-pid");
		auto instances = QsPaths::collectInstances(path);

		auto itr =
		    std::remove_if(instances.begin(), instances.end(), [&](const InstanceLockInfo& info) {
			    return !info.instance.instanceId.startsWith(*cmd.instance.id);
		    });

		instances.erase(itr, instances.end());

		if (instances.isEmpty()) {
			qCInfo(logBare) << "No running instances start with" << *cmd.instance.id;
			return -1;
		} else if (instances.length() != 1) {
			qCInfo(logBare) << "More than one instance starts with" << *cmd.instance.id;

			for (auto& instance: instances) {
				qCInfo(logBare).noquote() << " -" << instance.instance.instanceId;
			}

			return -1;
		} else {
			*instance = instances.value(0);
		}
	} else {
		QString configFilePath;
		auto r = locateConfigFile(cmd, configFilePath);
		if (r != 0) return r;

		auto pathId =
		    QCryptographicHash::hash(configFilePath.toUtf8(), QCryptographicHash::Md5).toHex();

		path = QDir(basePath->filePath("by-path")).filePath(pathId);

		auto instances = QsPaths::collectInstances(path);
		sortInstances(instances);

		if (instances.isEmpty()) {
			qCInfo(logBare) << "No running instances for" << configFilePath;
			return -1;
		}

		*instance = instances.value(0);
	}

	return 0;
}

int readLogFile(CommandState& cmd) {
	auto path = *cmd.log.file;

	if (path.isEmpty()) {
		InstanceLockInfo instance;
		auto r = selectInstance(cmd, &instance);
		if (r != 0) return r;

		path = QDir(QsPaths::basePath(instance.instance.instanceId)).filePath("log.qslog");
	}

	auto file = QFile(path);
	if (!file.open(QFile::ReadOnly)) {
		qCCritical(logBare) << "Failed to open log file" << path;
		return -1;
	}

	return qs::log::readEncodedLogs(
	           &file,
	           path,
	           cmd.log.timestamp,
	           cmd.log.tail,
	           cmd.log.follow,
	           *cmd.log.readoutRules
	       )
	         ? 0
	         : -1;
}

int listInstances(CommandState& cmd) {
	auto* basePath = QsPaths::instance()->baseRunDir();
	if (!basePath) return -1; // NOLINT

	QString path;
	QString configFilePath;
	if (cmd.instance.all) {
		path = basePath->filePath("by-pid");
	} else {
		auto r = locateConfigFile(cmd, configFilePath);

		if (r != 0) {
			qCInfo(logBare) << "Use --all to list all instances.";
			return r;
		}

		auto pathId =
		    QCryptographicHash::hash(configFilePath.toUtf8(), QCryptographicHash::Md5).toHex();

		path = QDir(basePath->filePath("by-path")).filePath(pathId);
	}

	auto instances = QsPaths::collectInstances(path);

	if (instances.isEmpty()) {
		if (cmd.instance.all) {
			qCInfo(logBare) << "No running instances.";
		} else {
			qCInfo(logBare) << "No running instances for" << configFilePath;
			qCInfo(logBare) << "Use --all to list all instances.";
		}
	} else {
		sortInstances(instances);

		if (cmd.output.json) {
			auto array = QJsonArray();

			for (auto& instance: instances) {
				auto json = QJsonObject();

				json["id"] = instance.instance.instanceId;
				json["pid"] = instance.pid;
				json["shell_id"] = instance.instance.shellId;
				json["config_path"] = instance.instance.configPath;
				json["launch_time"] = instance.instance.launchTime.toString(Qt::ISODate);

				array.push_back(json);
			}

			auto document = QJsonDocument(array);
			QTextStream(stdout) << document.toJson(QJsonDocument::Indented);
		} else {
			for (auto& instance: instances) {
				auto launchTimeStr = instance.instance.launchTime.toString("yyyy-MM-dd hh:mm:ss");

				auto runSeconds = instance.instance.launchTime.secsTo(QDateTime::currentDateTime());
				auto remSeconds = runSeconds % 60;
				auto runMinutes = (runSeconds - remSeconds) / 60;
				auto remMinutes = runMinutes % 60;
				auto runHours = (runMinutes - remMinutes) / 60;
				auto runtimeStr = QString("%1 hours, %2 minutes, %3 seconds")
				                      .arg(runHours)
				                      .arg(remMinutes)
				                      .arg(remSeconds);

				qCInfo(logBare).noquote().nospace()
				    << "Instance " << instance.instance.instanceId << ":\n"
				    << "  Process ID: " << instance.pid << '\n'
				    << "  Shell ID: " << instance.instance.shellId << '\n'
				    << "  Config path: " << instance.instance.configPath << '\n'
				    << "  Launch time: " << launchTimeStr << " (running for " << runtimeStr << ")\n";
			}
		}
	}

	return 0;
}

int killInstances(CommandState& cmd) {
	InstanceLockInfo instance;
	auto r = selectInstance(cmd, &instance);
	if (r != 0) return r;

	return IpcClient::connect(instance.instance.instanceId, [&](IpcClient& client) {
		client.kill();
		qCInfo(logBare).noquote() << "Killed" << instance.instance.instanceId;
	});
}

int msgInstance(CommandState& cmd) {
	InstanceLockInfo instance;
	auto r = selectInstance(cmd, &instance);
	if (r != 0) return r;

	return IpcClient::connect(instance.instance.instanceId, [&](IpcClient& client) {
		if (cmd.ipc.info) {
			return qs::io::ipc::comm::queryMetadata(&client, *cmd.ipc.target, *cmd.ipc.function);
		} else {
			QVector<QString> arguments;
			for (auto& arg: cmd.ipc.arguments) {
				arguments += *arg;
			}

			return qs::io::ipc::comm::callFunction(
			    &client,
			    *cmd.ipc.target,
			    *cmd.ipc.function,
			    arguments
			);
		}

		return -1;
	});
}

template <typename T>
QString base36Encode(T number) {
	const QString digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	QString result;

	do {
		result.prepend(digits[number % 36]);
		number /= 36;
	} while (number > 0);

	for (auto i = 0; i < result.length() / 2; i++) {
		auto opposite = result.length() - i - 1;
		auto c = result.at(i);
		result[i] = result.at(opposite);
		result[opposite] = c;
	}

	return result;
}

void checkCrashRelaunch(char** argv, QCoreApplication* coreApplication) {
#if CRASH_REPORTER
	auto lastInfoFdStr = qEnvironmentVariable("__QUICKSHELL_CRASH_INFO_FD");

	if (!lastInfoFdStr.isEmpty()) {
		auto lastInfoFd = lastInfoFdStr.toInt();

		QFile file;
		file.open(lastInfoFd, QFile::ReadOnly, QFile::AutoCloseHandle);
		file.seek(0);

		auto ds = QDataStream(&file);
		RelaunchInfo info;
		ds >> info;

		LogManager::init(
		    !info.noColor,
		    info.timestamp,
		    info.sparseLogsOnly,
		    info.defaultLogLevel,
		    info.logRules
		);

		qCritical().nospace() << "Quickshell has crashed under pid "
		                      << qEnvironmentVariable("__QUICKSHELL_CRASH_DUMP_PID").toInt()
		                      << " (Coredumps will be available under that pid.)";

		qCritical() << "Further crash information is stored under"
		            << QsPaths::crashDir(info.instance.instanceId).path();

		if (info.instance.launchTime.msecsTo(QDateTime::currentDateTime()) < 10000) {
			qCritical() << "Quickshell crashed within 10 seconds of launching. Not restarting to avoid "
			               "a crash loop.";
			exit(-1); // NOLINT
		} else {
			qCritical() << "Quickshell has been restarted.";

			launch({.configPath = info.instance.configPath}, argv, coreApplication);
		}
	}
#endif
}

int launchFromCommand(CommandState& cmd, QCoreApplication* coreApplication) {
	QString configPath;

	auto r = locateConfigFile(cmd, configPath);
	if (r != 0) return r;

	{
		InstanceLockInfo info;
		if (cmd.misc.noDuplicate && selectInstance(cmd, &info) == 0) {
			qCInfo(logBare) << "An instance of this configuration is already running.";
			return 0;
		}
	}

	return launch(
	    {
	        .configPath = configPath,
	        .debugPort = cmd.debug.port,
	        .waitForDebug = cmd.debug.wait,
	    },
	    cmd.exec.argv,
	    coreApplication
	);
}

int launch(const LaunchArgs& args, char** argv, QCoreApplication* coreApplication) {
	auto pathId = QCryptographicHash::hash(args.configPath.toUtf8(), QCryptographicHash::Md5).toHex();
	auto shellId = QString(pathId);

	qInfo() << "Launching config:" << args.configPath;

	auto file = QFile(args.configPath);
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		qCritical() << "Could not open config file" << args.configPath;
		return -1;
	}

	struct {
		bool useQApplication = false;
		bool nativeTextRendering = false;
		bool desktopSettingsAware = true;
		QHash<QString, QString> envOverrides;
	} pragmas;

	auto stream = QTextStream(&file);
	while (!stream.atEnd()) {
		auto line = stream.readLine().trimmed();
		if (line.startsWith("//@ pragma ")) {
			auto pragma = line.sliced(11).trimmed();

			if (pragma == "UseQApplication") pragmas.useQApplication = true;
			else if (pragma == "NativeTextRendering") pragmas.nativeTextRendering = true;
			else if (pragma == "IgnoreSystemSettings") pragmas.desktopSettingsAware = false;
			else if (pragma.startsWith("Env ")) {
				auto envPragma = pragma.sliced(4);
				auto splitIdx = envPragma.indexOf('=');

				if (splitIdx == -1) {
					qCritical() << "Env pragma" << pragma << "not in the form 'VAR = VALUE'";
					return -1;
				}

				auto var = envPragma.sliced(0, splitIdx).trimmed();
				auto val = envPragma.sliced(splitIdx + 1).trimmed();
				pragmas.envOverrides.insert(var, val);
			} else if (pragma.startsWith("ShellId ")) {
				shellId = pragma.sliced(8).trimmed();
			} else {
				qCritical() << "Unrecognized pragma" << pragma;
				return -1;
			}
		} else if (line.startsWith("import")) break;
	}

	file.close();

	qInfo() << "Shell ID:" << shellId << "Path ID" << pathId;

	auto launchTime = qs::Common::LAUNCH_TIME.toSecsSinceEpoch();
	InstanceInfo::CURRENT = InstanceInfo {
	    .instanceId = base36Encode(getpid()) + base36Encode(launchTime),
	    .configPath = args.configPath,
	    .shellId = shellId,
	    .launchTime = qs::Common::LAUNCH_TIME,
	};

#if CRASH_REPORTER
	auto crashHandler = crash::CrashHandler();
	crashHandler.init();

	{
		auto* log = LogManager::instance();
		crashHandler.setRelaunchInfo({
		    .instance = InstanceInfo::CURRENT,
		    .noColor = !log->colorLogs,
		    .timestamp = log->timestampLogs,
		    .sparseLogsOnly = log->isSparse(),
		    .defaultLogLevel = log->defaultLevel(),
		    .logRules = log->rulesString(),
		});
	}
#endif

	QsPaths::init(shellId, pathId);
	QsPaths::instance()->linkRunDir();
	QsPaths::instance()->linkPathDir();
	LogManager::initFs();

	for (auto [var, val]: pragmas.envOverrides.asKeyValueRange()) {
		qputenv(var.toUtf8(), val.toUtf8());
	}

	// The qml engine currently refuses to cache non file (qsintercept) paths.

	// if (auto* cacheDir = QsPaths::instance()->cacheDir()) {
	// 	auto qmlCacheDir = cacheDir->filePath("qml-engine-cache");
	// 	qputenv("QML_DISK_CACHE_PATH", qmlCacheDir.toLocal8Bit());
	//
	// 	if (!qEnvironmentVariableIsSet("QML_DISK_CACHE")) {
	// 		qputenv("QML_DISK_CACHE", "aot,qmlc");
	// 	}
	// }

	// While the simple animation driver can lead to better animations in some cases,
	// it also can cause excessive repainting at excessively high framerates which can
	// lead to noticeable amounts of gpu usage, including overheating on some systems.
	// This gets worse the more windows are open, as repaints trigger on all of them for
	// some reason. See QTBUG-126099 for details.

	// if (!qEnvironmentVariableIsSet("QSG_USE_SIMPLE_ANIMATION_DRIVER")) {
	// 	qputenv("QSG_USE_SIMPLE_ANIMATION_DRIVER", "1");
	// }

	// Some programs place icons in the pixmaps folder instead of the icons folder.
	// This seems to be controlled by the QPA and qt6ct does not provide it.
	{
		QList<QString> dataPaths;

		if (qEnvironmentVariableIsSet("XDG_DATA_DIRS")) {
			auto var = qEnvironmentVariable("XDG_DATA_DIRS");
			dataPaths = var.split(u':', Qt::SkipEmptyParts);
		} else {
			dataPaths.push_back("/usr/local/share");
			dataPaths.push_back("/usr/share");
		}

		auto fallbackPaths = QIcon::fallbackSearchPaths();

		for (auto& path: dataPaths) {
			auto newPath = QDir(path).filePath("pixmaps");

			if (!fallbackPaths.contains(newPath)) {
				fallbackPaths.push_back(newPath);
			}
		}

		QIcon::setFallbackSearchPaths(fallbackPaths);
	}

	QGuiApplication::setDesktopSettingsAware(pragmas.desktopSettingsAware);

	delete coreApplication;

	QGuiApplication* app = nullptr;
	auto qArgC = 0;

	if (pragmas.useQApplication) {
		app = new QApplication(qArgC, argv);
	} else {
		app = new QGuiApplication(qArgC, argv);
	}

	if (args.debugPort != -1) {
		QQmlDebuggingEnabler::enableDebugging(true);
		auto wait = args.waitForDebug ? QQmlDebuggingEnabler::WaitForClient
		                              : QQmlDebuggingEnabler::DoNotWaitForClient;
		QQmlDebuggingEnabler::startTcpDebugServer(args.debugPort, wait);
	}

	QuickshellPlugin::initPlugins();

	// Base window transparency appears to be additive.
	// Use a fully transparent window with a colored rect.
	QQuickWindow::setDefaultAlphaBuffer(true);

	if (pragmas.nativeTextRendering) {
		QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);
	}

	qs::ipc::IpcServer::start();
	QsPaths::instance()->createLock();

	auto root = RootWrapper(args.configPath, shellId);
	QGuiApplication::setQuitOnLastWindowClosed(false);

	exitDaemon(0);

	auto code = QGuiApplication::exec();
	delete app;
	return code;
}

} // namespace qs::launch
