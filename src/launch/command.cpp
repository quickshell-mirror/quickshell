#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <utility>

#include <qconfig.h>
#include <qcontainerfwd.h>
#include <qcoreapplication.h>
#include <qcryptographichash.h>
#include <qdatetime.h>
#include <qdebug.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qstandardpaths.h>
#include <qtenvironmentvariables.h>
#include <qtversion.h>
#include <unistd.h>

#include "../core/instanceinfo.hpp"
#include "../core/logging.hpp"
#include "../core/paths.hpp"
#include "../io/ipccomm.hpp"
#include "../ipc/ipc.hpp"
#include "build.hpp"
#include "launch_p.hpp"

namespace qs::launch {

using qs::ipc::IpcClient;

namespace {

QList<QString> configBaseDirs() {
	auto configHome = qEnvironmentVariable("XDG_CONFIG_HOME");
	if (configHome.isEmpty()) {
		auto home = QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
		configHome = QDir(home).filePath(".config");
	}

	auto configDirs = qEnvironmentVariable("XDG_CONFIG_DIRS").split(':', Qt::SkipEmptyParts);
	if (configDirs.isEmpty()) {
		configDirs.append("/etc/xdg");
	}

	configDirs.prepend(configHome);

	for (auto& dir: configDirs) {
		dir.append("/quickshell");
	}

	return configDirs;
}

QString locateNamedConfig(const QString& name) {
	for (const auto& baseDir: configBaseDirs()) {
		auto shellPath = QDir(baseDir).filePath("shell.qml");
		auto hasShell = QFileInfo(shellPath).isFile();

		if (hasShell) {
			if (name == "default") return shellPath;
			else continue; // skip subfolders if shell.qml is present in folder
		}

		shellPath = QDir(QDir(baseDir).filePath(name)).filePath("shell.qml");
		if (QFileInfo(shellPath).isFile()) return shellPath;
	}

	return QString();
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
			qWarning(
			) << "Config manifests (manifest.conf) are deprecated and will be removed in a future "
			     "release.";
			qWarning() << "Consider using symlinks to a subfolder of quickshell's XDG config dirs.";

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
			const auto& name = cmd.config.name->isEmpty() ? "default" : *cmd.config.name;
			path = locateNamedConfig(name);

			if (path.isEmpty()) {
				if (name == "default") {
					qCCritical(logBare
					) << "Could not find \"default\" config directory or shell.qml in any valid config path.";
				} else {
					qCCritical(logBare) << "Could not find" << name
					                    << "config directory in any valid config path.";
				}

				return -1;
			}

			path = QFileInfo(path).canonicalFilePath();
			return 0;
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

void sortInstances(QVector<InstanceLockInfo>& list, bool newestFirst) {
	std::ranges::sort(list, [=](const InstanceLockInfo& a, const InstanceLockInfo& b) {
		auto r = a.instance.launchTime < b.instance.launchTime;
		return newestFirst ? !r : r;
	});
};

int selectInstance(CommandState& cmd, InstanceLockInfo* instance, bool deadFallback = false) {
	auto* basePath = QsPaths::instance()->baseRunDir();
	if (!basePath) return -1;

	QString path;

	if (cmd.instance.pid != -1) {
		path = QDir(basePath->filePath("by-pid")).filePath(QString::number(cmd.instance.pid));
		if (!QsPaths::checkLock(path, instance, deadFallback)) {
			qCInfo(logBare) << "No instance found for pid" << cmd.instance.pid;
			return -1;
		}
	} else if (!cmd.instance.id->isEmpty()) {
		path = basePath->filePath("by-pid");
		auto [liveInstances, deadInstances] = QsPaths::collectInstances(path);

		liveInstances.removeIf([&](const InstanceLockInfo& info) {
			return !info.instance.instanceId.startsWith(*cmd.instance.id);
		});

		deadInstances.removeIf([&](const InstanceLockInfo& info) {
			return !info.instance.instanceId.startsWith(*cmd.instance.id);
		});

		auto instances = liveInstances.isEmpty() && deadFallback ? deadInstances : liveInstances;

		if (instances.isEmpty()) {
			if (deadFallback) {
				qCInfo(logBare) << "No instances start with" << *cmd.instance.id;
			} else {
				qCInfo(logBare) << "No running instances start with" << *cmd.instance.id;

				if (!deadInstances.isEmpty()) {
					qCInfo(logBare) << "Some dead instances match:";

					for (auto& instance: deadInstances) {
						qCInfo(logBare).noquote() << " -" << instance.instance.instanceId;
					}
				}
			}

			return -1;
		} else if (instances.length() != 1) {
			qCInfo(logBare) << "More than one instance starts with" << *cmd.instance.id;

			for (auto& instance: instances) {
				qCInfo(logBare).noquote() << " -" << instance.instance.instanceId
				                          << (instance.pid == -1 ? " (dead)" : "");
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

		auto [liveInstances, deadInstances] = QsPaths::collectInstances(path);

		auto instances = liveInstances;
		if (instances.isEmpty() && deadFallback) {
			instances = deadInstances;
		}

		sortInstances(
		    instances,
		    cmd.config.newest || (!instances.empty() && instances.first().pid == -1)
		);

		if (instances.isEmpty()) {
			if (liveInstances.isEmpty() && deadInstances.length() > 1) {
				qCInfo(logBare) << "No running instances for" << configFilePath;
				qCInfo(logBare) << "Dead instances:";
				sortInstances(deadInstances, cmd.config.newest);
				for (auto& instance: deadInstances) {
					qCInfo(logBare).noquote() << " -" << instance.instance.instanceId;
				}
			} else {
				qCInfo(logBare) << "No running instances for" << configFilePath;
			}
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
		auto r = selectInstance(cmd, &instance, true);
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

	auto [liveInstances, deadInstances] = QsPaths::collectInstances(path);

	sortInstances(liveInstances, cmd.config.newest);

	QList<InstanceLockInfo> instances;
	if (cmd.instance.includeDead) {
		sortInstances(deadInstances, cmd.config.newest);
		instances = std::move(deadInstances);
		instances.append(liveInstances);
	} else {
		instances = std::move(liveInstances);
	}

	if (instances.isEmpty()) {
		if (cmd.instance.all) {
			qCInfo(logBare) << "No running instances.";
		} else {
			qCInfo(logBare) << "No running instances for" << configFilePath;
			qCInfo(logBare) << "Use --all to list all instances.";
		}
	} else {

		if (cmd.output.json) {
			auto array = QJsonArray();

			for (auto& instance: instances) {
				auto json = QJsonObject();

				json["id"] = instance.instance.instanceId;
				json["pid"] = instance.instance.pid;
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

				auto isDead = instance.pid == -1;
				auto gray = !cmd.log.noColor && isDead;

				qCInfo(logBare).noquote().nospace()
				    << (gray ? "\033[90m" : "") << "Instance " << instance.instance.instanceId
				    << (isDead ? " (dead)" : "") << ":\n"
				    << "  Process ID: " << instance.instance.pid << '\n'
				    << "  Shell ID: " << instance.instance.shellId << '\n'
				    << "  Config path: " << instance.instance.configPath << '\n'
				    << "  Launch time: " << launchTimeStr
				    << (isDead ? "" : " (running for " + runtimeStr + ")") << '\n'
				    << (gray ? "\033[0m" : "");
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

int ipcCommand(CommandState& cmd) {
	InstanceLockInfo instance;
	auto r = selectInstance(cmd, &instance);
	if (r != 0) return r;

	return IpcClient::connect(instance.instance.instanceId, [&](IpcClient& client) {
		if (*cmd.ipc.show || cmd.ipc.showOld) {
			return qs::io::ipc::comm::queryMetadata(&client, *cmd.ipc.target, *cmd.ipc.name);
		} else if (*cmd.ipc.getprop) {
			return qs::io::ipc::comm::getProperty(&client, *cmd.ipc.target, *cmd.ipc.name);
		} else {
			QVector<QString> arguments;
			for (auto& arg: cmd.ipc.arguments) {
				arguments += *arg;
			}

			return qs::io::ipc::comm::callFunction(&client, *cmd.ipc.target, *cmd.ipc.name, arguments);
		}

		return -1;
	});
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

} // namespace

int runCommand(int argc, char** argv, QCoreApplication* coreApplication) {
	auto state = CommandState();
	if (auto ret = parseCommand(argc, argv, state); ret != 65535) return ret;

	if (state.misc.checkCompat) {
		if (strcmp(qVersion(), QT_VERSION_STR) != 0) {
			QTextStream(stdout) << "\033[31mCOMPATIBILITY WARNING: Quickshell was built against Qt "
			                    << QT_VERSION_STR << " but the system has updated to Qt " << qVersion()
			                    << " without rebuilding the package. This is likely to cause crashes, so "
			                       "you must rebuild the quickshell package.\n";
			return 1;
		}

		return 0;
	}

	// Has to happen before extra threads are spawned.
	if (state.misc.daemonize) {
		auto closepipes = std::array<int, 2>();
		if (pipe(closepipes.data()) == -1) {
			qFatal().nospace() << "Failed to create messaging pipes for daemon with error " << errno
			                   << ": " << qt_error_string();
		}

		pid_t pid = fork(); // NOLINT (include)

		if (pid == -1) {
			qFatal().nospace() << "Failed to fork daemon with error " << errno << ": "
			                   << qt_error_string();
		} else if (pid == 0) {
			DAEMON_PIPE = closepipes[1];
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
		qCInfo(logBare).noquote().nospace()
		    << "quickshell 0.2.0, revision " << GIT_REVISION << ", distributed by: " << DISTRIBUTOR;

		if (state.log.verbosity > 1) {
			qCInfo(logBare).noquote() << "\nBuildtime Qt Version:" << QT_VERSION_STR;
			qCInfo(logBare).noquote() << "Runtime Qt Version:" << qVersion();
			qCInfo(logBare).noquote() << "Compiler:" << COMPILER;
			qCInfo(logBare).noquote() << "Compile Flags:" << COMPILE_FLAGS;
		}

		if (state.log.verbosity > 0) {
			qCInfo(logBare).noquote() << "\nBuild Type:" << BUILD_TYPE;
			qCInfo(logBare).noquote() << "Build configuration:";
			qCInfo(logBare).noquote().nospace() << BUILD_CONFIGURATION;
		}
	} else if (*state.subcommand.log) {
		return readLogFile(state);
	} else if (*state.subcommand.list) {
		return listInstances(state);
	} else if (*state.subcommand.kill) {
		return killInstances(state);
	} else if (*state.subcommand.msg || *state.ipc.ipc) {
		return ipcCommand(state);
	} else {
		if (strcmp(qVersion(), QT_VERSION_STR) != 0) {
			qWarning() << "\033[31mQuickshell was built against Qt" << QT_VERSION_STR
			           << "but the system has updated to Qt" << qVersion()
			           << "without rebuilding the package. This is likely to cause crashes, so "
			              "the quickshell package must be rebuilt.\n";
		}

		return launchFromCommand(state, coreApplication);
	}

	return 0;
}

} // namespace qs::launch
