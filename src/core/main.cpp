#include "main.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

#include <CLI/App.hpp>
#include <CLI/CLI.hpp> // NOLINT: Need to include this for impls of some CLI11 classes
#include <CLI/Error.hpp>
#include <CLI/Validators.hpp>
#include <qapplication.h>
#include <qcoreapplication.h>
#include <qcryptographichash.h>
#include <qdatastream.h>
#include <qdatetime.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qguiapplication.h>
#include <qhash.h>
#include <qicon.h>
#include <qlist.h>
#include <qlogging.h>
#include <qnamespace.h>
#include <qqmldebug.h>
#include <qquickwindow.h>
#include <qstandardpaths.h>
#include <qstring.h>
#include <qtenvironmentvariables.h>
#include <qtextstream.h>
#include <qtpreprocessorsupport.h>

#include "build.hpp"
#include "common.hpp"
#include "crashinfo.hpp"
#include "logging.hpp"
#include "paths.hpp"
#include "plugin.hpp"
#include "rootwrapper.hpp"
#if CRASH_REPORTER
#include "../crash/handler.hpp"
#include "../crash/main.hpp"
#endif

struct CommandInfo {
	QString configPath;
	QString manifestPath;
	QString configName;
	QString& initialWorkdir;
	int& debugPort;
	bool& waitForDebug;
	bool& printInfo;
	bool& noColor;
	bool& sparseLogsOnly;
};

void processCommand(int argc, char** argv, CommandInfo& info) {
	auto app = CLI::App("");

	class QStringOption {
	public:
		QStringOption() = default;
		QStringOption& operator=(const std::string& str) {
			this->str = QString::fromStdString(str);
			return *this;
		}

		QString& operator*() { return this->str; }

	private:
		QString str;
	};

	class QStringRefOption {
	public:
		QStringRefOption(QString* str): str(str) {}
		QStringRefOption& operator=(const std::string& str) {
			*this->str = QString::fromStdString(str);
			return *this;
		}

	private:
		QString* str;
	};

	/// ---
	QStringRefOption path(&info.configPath);
	QStringRefOption manifest(&info.manifestPath);
	QStringRefOption config(&info.configName);
	QStringRefOption workdirRef(&info.initialWorkdir);

	auto* selection = app.add_option_group(
	    "Config Selection",
	    "Select a configuration to run (defaults to $XDG_CONFIG_HOME/quickshell/shell.qml)"
	);

	auto* pathArg =
	    selection->add_option("-p,--path", path, "Path to a QML file to run. (Env:QS_CONFIG_PATH)");

	auto* mfArg = selection->add_option(
	    "-m,--manifest",
	    manifest,
	    "Path to a manifest containing configurations. (Env:QS_MANIFEST)\n"
	    "(Defaults to $XDG_CONFIG_HOME/quickshell/manifest.conf)"
	);

	auto* cfgArg = selection->add_option(
	    "-c,--config",
	    config,
	    "Name of a configuration within a manifest. (Env:QS_CONFIG_NAME)"
	);

	selection->add_option("-d,--workdir", workdirRef, "Initial working directory.");

	pathArg->excludes(mfArg, cfgArg);

	/// ---
	auto* debug = app.add_option_group("Debugging");

	auto* debugPortArg = debug
	                         ->add_option(
	                             "--debugport",
	                             info.debugPort,
	                             "Open the given port for a QML debugger to connect to."
	                         )
	                         ->check(CLI::Range(0, 65535));

	debug
	    ->add_flag(
	        "--waitfordebug",
	        info.waitForDebug,
	        "Wait for a debugger to attach to the given port before launching."
	    )
	    ->needs(debugPortArg);

	/// ---
	app.add_flag("--info", info.printInfo, "Print information about the shell")
	    ->excludes(debugPortArg);
	app.add_flag("--no-color", info.noColor, "Do not color the log output. (Env:NO_COLOR)");
	auto* printVersion = app.add_flag("-V,--version", "Print quickshell's version, then exit.");

	app.add_flag(
	    "--no-detailed-logs",
	    info.sparseLogsOnly,
	    "Do not enable this unless you know what you are doing."
	);

	/// ---
	QStringOption logPath;
	QStringOption logFilter;
	auto logNoTime = false;

	auto* readLog = app.add_subcommand("read-log", "Read a quickshell log file.");
	readLog->add_option("path", logPath, "Path to the log file to read")->required();

	readLog->add_option(
	    "-f,--filter",
	    logFilter,
	    "Logging categories to display. (same syntax as QT_LOGGING_RULES)"
	);

	readLog->add_flag("--no-time", logNoTime, "Do not print timestamps of log messages.");
	readLog->add_flag("--no-color", info.noColor, "Do not color the log output. (Env:NO_COLOR)");

	try {
		app.parse(argc, argv);
	} catch (const CLI::ParseError& e) {
		exit(app.exit(e)); // NOLINT
	};

	if (*printVersion) {
		std::cout << "quickshell pre-release, revision: " << GIT_REVISION << std::endl;
		exit(0); // NOLINT
	} else if (*readLog) {
		auto file = QFile(*logPath);
		if (!file.open(QFile::ReadOnly)) {
			qCritical() << "Failed to open log for reading:" << *logPath;
			exit(-1); // NOLINT
		} else {
			qInfo() << "Reading log" << *logPath;
		}

		exit( // NOLINT
		    qs::log::readEncodedLogs(&file, !logNoTime, *logFilter) ? 0 : -1
		);
	}
}

QString commandConfigPath(QString path, QString manifest, QString config, bool printInfo) {
	// NOLINTBEGIN
#define CHECK(rname, name, level, label, expr)                                                     \
	QString name = expr;                                                                             \
	if (rname.isEmpty() && !name.isEmpty()) {                                                        \
		rname = name;                                                                                  \
		rname##Level = level;                                                                          \
		if (!printInfo) goto label;                                                                    \
	}

#define OPTSTR(name) (name.isEmpty() ? "(unset)" : name.toStdString())
	// NOLINTEND

	QString basePath;
	int basePathLevel = 0;
	Q_UNUSED(basePathLevel);
	{
		// NOLINTBEGIN
		// clang-format off
		CHECK(basePath, envBasePath, 0, foundbase, qEnvironmentVariable("QS_BASE_PATH"));
		CHECK(basePath, defaultBasePath, 0, foundbase, QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)).filePath("quickshell"));
		// clang-format on
		// NOLINTEND

		if (printInfo) {
			// clang-format off
			std::cout << "Base path: " << OPTSTR(basePath) << "\n";
			std::cout << " - Environment (QS_BASE_PATH): " << OPTSTR(envBasePath) << "\n";
			std::cout << " - Default: " << OPTSTR(defaultBasePath) << "\n";
			// clang-format on
		}
	}
foundbase:;

	QString configPath;
	int configPathLevel = 10;
	{
		// NOLINTBEGIN
		CHECK(configPath, optionConfigPath, 0, foundpath, path);
		CHECK(configPath, envConfigPath, 1, foundpath, qEnvironmentVariable("QS_CONFIG_PATH"));
		// NOLINTEND

		if (printInfo) {
			// clang-format off
			std::cout << "\nConfig path: " << OPTSTR(configPath) << "\n";
			std::cout << " - Option: " << OPTSTR(optionConfigPath) << "\n";
			std::cout << " - Environment (QS_CONFIG_PATH): " << OPTSTR(envConfigPath) << "\n";
			// clang-format on
		}
	}
foundpath:;

	QString manifestPath;
	int manifestPathLevel = 10;
	{
		// NOLINTBEGIN
		// clang-format off
		CHECK(manifestPath, optionManifestPath, 0, foundmf, manifest);
		CHECK(manifestPath, envManifestPath, 1, foundmf, qEnvironmentVariable("QS_MANIFEST"));
		CHECK(manifestPath, defaultManifestPath, 2, foundmf, QDir(basePath).filePath("manifest.conf"));
		// clang-format on
		// NOLINTEND

		if (printInfo) {
			// clang-format off
			std::cout << "\nManifest path: " << OPTSTR(manifestPath) << "\n";
			std::cout << " - Option: " << OPTSTR(optionManifestPath) << "\n";
			std::cout << " - Environment (QS_MANIFEST): " << OPTSTR(envManifestPath) << "\n";
			std::cout << " - Default: " << OPTSTR(defaultManifestPath) << "\n";
			// clang-format on
		}
	}
foundmf:;

	QString configName;
	int configNameLevel = 10;
	{
		// NOLINTBEGIN
		CHECK(configName, optionConfigName, 0, foundname, config);
		CHECK(configName, envConfigName, 1, foundname, qEnvironmentVariable("QS_CONFIG_NAME"));
		// NOLINTEND

		if (printInfo) {
			// clang-format off
			std::cout << "\nConfig name: " << OPTSTR(configName) << "\n";
			std::cout << " - Option: " << OPTSTR(optionConfigName) << "\n";
			std::cout << " - Environment (QS_CONFIG_NAME): " << OPTSTR(envConfigName) << "\n\n";
			// clang-format on
		}
	}
foundname:;

	QString configFilePath;

	if (!configPath.isEmpty() && configPathLevel <= configNameLevel) {
		configFilePath = configPath;
	} else if (!configName.isEmpty()) {
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
						qCritical() << "manifest line not in expected format 'name = relativepath':" << line;
						exit(-1); // NOLINT
					}

					if (split[0].trimmed() == configName) {
						configFilePath = QDir(QFileInfo(file).canonicalPath()).filePath(split[1].trimmed());
						goto foundp;
					}
				}

				qCritical() << "configuration" << configName << "not found in manifest" << manifestPath;
				exit(-1); // NOLINT
			} else if (manifestPathLevel < 2) {
				qCritical() << "cannot open config manifest at" << manifestPath;
				exit(-1); // NOLINT
			}
		}

		{
			auto basePathInfo = QFileInfo(basePath);
			if (!basePathInfo.exists()) {
				qCritical() << "base path does not exist:" << basePath;
				exit(-1); // NOLINT
			} else if (!QFileInfo(basePathInfo.canonicalFilePath()).isDir()) {
				qCritical() << "base path is not a directory" << basePath;
				exit(-1); // NOLINT
			}

			auto dir = QDir(basePath);
			for (auto& entry: dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot)) {
				if (entry == configName) {
					configFilePath = dir.filePath(entry);
					goto foundp;
				}
			}

			qCritical() << "no directory named " << configName << "found in base path" << basePath;
			exit(-1); // NOLINT
		}
	} else {
		configFilePath = basePath;
	}

foundp:;
	auto configFile = QFileInfo(configFilePath);
	if (!configFile.exists()) {
		qCritical() << "config path does not exist:" << configFilePath;
		exit(-1); // NOLINT
	}

	if (configFile.isDir()) {
		configFilePath = QDir(configFilePath).filePath("shell.qml");
	}

	configFile = QFileInfo(configFilePath);
	if (!configFile.exists()) {
		qCritical() << "no shell.qml found in config path:" << configFilePath;
		exit(-1); // NOLINT
	} else if (configFile.isDir()) {
		qCritical() << "shell.qml is a directory:" << configFilePath;
		exit(-1); // NOLINT
	}

	configFilePath = QFileInfo(configFilePath).canonicalFilePath();
	configFile = QFileInfo(configFilePath);
	if (!configFile.exists()) {
		qCritical() << "config file does not exist:" << configFilePath;
		exit(-1); // NOLINT
	} else if (configFile.isDir()) {
		qCritical() << "config file is a directory:" << configFilePath;
		exit(-1); // NOLINT
	}

#undef CHECK
#undef OPTSTR

	return configFilePath;
}

int qs_main(int argc, char** argv) {
#if CRASH_REPORTER
	qsCheckCrash(argc, argv);
	auto crashHandler = qs::crash::CrashHandler();
#endif

	auto qArgC = 1;
	auto* qArgV = argv;

	QString configFilePath;
	QString initialWorkdir;
	QString shellId;

	int debugPort = -1;
	bool waitForDebug = false;
	bool printInfo = false;
	bool noColor = !qEnvironmentVariableIsEmpty("NO_COLOR");
	bool sparseLogsOnly = false;

	auto useQApplication = false;
	auto nativeTextRendering = false;
	auto desktopSettingsAware = true;
	QHash<QString, QString> envOverrides;

	{
		const auto qApplication = QCoreApplication(qArgC, qArgV);

#if CRASH_REPORTER
		auto lastInfoFdStr = qEnvironmentVariable("__QUICKSHELL_CRASH_INFO_FD");

		if (!lastInfoFdStr.isEmpty()) {
			auto lastInfoFd = lastInfoFdStr.toInt();

			QFile file;
			file.open(lastInfoFd, QFile::ReadOnly, QFile::AutoCloseHandle);
			file.seek(0);

			auto ds = QDataStream(&file);
			InstanceInfo info;
			ds >> info;

			configFilePath = info.configPath;
			initialWorkdir = info.initialWorkdir;
			noColor = info.noColor;
			sparseLogsOnly = info.sparseLogsOnly;

			LogManager::init(!noColor, sparseLogsOnly);

			qCritical().nospace() << "Quickshell has crashed under pid "
			                      << qEnvironmentVariable("__QUICKSHELL_CRASH_DUMP_PID").toInt()
			                      << " (Coredumps will be available under that pid.)";

			qCritical() << "Further crash information is stored under"
			            << QsPaths::crashDir(info.shellId, info.launchTime).path();

			if (info.launchTime.msecsTo(QDateTime::currentDateTime()) < 10000) {
				qCritical() << "Quickshell crashed within 10 seconds of launching. Not restarting to avoid "
				               "a crash loop.";
				return 0;
			} else {
				qCritical() << "Quickshell has been restarted.";
			}

			crashHandler.init();
		} else
#endif
		{

			auto command = CommandInfo {
			    .initialWorkdir = initialWorkdir,
			    .debugPort = debugPort,
			    .waitForDebug = waitForDebug,
			    .printInfo = printInfo,
			    .noColor = noColor,
			    .sparseLogsOnly = sparseLogsOnly,
			};

			processCommand(argc, argv, command);

			// Start log manager - has to happen with an active event loop or offthread can't be started.
			LogManager::init(!noColor, sparseLogsOnly);

#if CRASH_REPORTER
			// Started after log manager for pretty debug logs. Unlikely anything will crash before this point, but
			// this can be moved if it happens.
			crashHandler.init();
#endif

			configFilePath = commandConfigPath(
			    command.configPath,
			    command.manifestPath,
			    command.configName,
			    command.printInfo
			);
		}

		shellId = QCryptographicHash::hash(configFilePath.toUtf8(), QCryptographicHash::Md5).toHex();

		qInfo() << "Config file path:" << configFilePath;

		if (!QFile(configFilePath).exists()) {
			qCritical() << "config file does not exist";
			return -1;
		}

		auto file = QFile(configFilePath);
		if (!file.open(QFile::ReadOnly | QFile::Text)) {
			qCritical() << "could not open config file";
			return -1;
		}

		auto stream = QTextStream(&file);
		while (!stream.atEnd()) {
			auto line = stream.readLine().trimmed();
			if (line.startsWith("//@ pragma ")) {
				auto pragma = line.sliced(11).trimmed();

				if (pragma == "UseQApplication") useQApplication = true;
				else if (pragma == "NativeTextRendering") nativeTextRendering = true;
				else if (pragma == "IgnoreSystemSettings") desktopSettingsAware = false;
				else if (pragma.startsWith("Env ")) {
					auto envPragma = pragma.sliced(4);
					auto splitIdx = envPragma.indexOf('=');

					if (splitIdx == -1) {
						qCritical() << "Env pragma" << pragma << "not in the form 'VAR = VALUE'";
						return -1;
					}

					auto var = envPragma.sliced(0, splitIdx).trimmed();
					auto val = envPragma.sliced(splitIdx + 1).trimmed();
					envOverrides.insert(var, val);
				} else if (pragma.startsWith("ShellId ")) {
					shellId = pragma.sliced(8).trimmed();
				} else {
					qCritical() << "Unrecognized pragma" << pragma;
					return -1;
				}
			} else if (line.startsWith("import")) break;
		}

		file.close();
	}

	qInfo() << "Shell ID:" << shellId;

	if (printInfo) return 0;

#if CRASH_REPORTER
	crashHandler.setInstanceInfo(InstanceInfo {
	    .configPath = configFilePath,
	    .shellId = shellId,
	    .initialWorkdir = initialWorkdir,
	    .launchTime = qs::Common::LAUNCH_TIME,
	    .noColor = noColor,
	    .sparseLogsOnly = sparseLogsOnly,
	});
#endif

	for (auto [var, val]: envOverrides.asKeyValueRange()) {
		qputenv(var.toUtf8(), val.toUtf8());
	}

	QsPaths::init(shellId);
	QsPaths::instance()->linkPidRunDir();

	if (auto* cacheDir = QsPaths::instance()->cacheDir()) {
		auto qmlCacheDir = cacheDir->filePath("qml-engine-cache");
		qputenv("QML_DISK_CACHE_PATH", qmlCacheDir.toLocal8Bit());

		if (!qEnvironmentVariableIsSet("QML_DISK_CACHE")) {
			qputenv("QML_DISK_CACHE", "aot,qmlc");
		}
	}

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

	QGuiApplication::setDesktopSettingsAware(desktopSettingsAware);

	QGuiApplication* app = nullptr;

	if (useQApplication) {
		app = new QApplication(qArgC, qArgV);
	} else {
		app = new QGuiApplication(qArgC, qArgV);
	}

	LogManager::initFs();

	if (debugPort != -1) {
		QQmlDebuggingEnabler::enableDebugging(true);
		auto wait = waitForDebug ? QQmlDebuggingEnabler::WaitForClient
		                         : QQmlDebuggingEnabler::DoNotWaitForClient;
		QQmlDebuggingEnabler::startTcpDebugServer(debugPort, wait);
	}

	if (!initialWorkdir.isEmpty()) {
		QDir::setCurrent(initialWorkdir);
	}

	QuickshellPlugin::initPlugins();

	// Base window transparency appears to be additive.
	// Use a fully transparent window with a colored rect.
	QQuickWindow::setDefaultAlphaBuffer(true);

	if (nativeTextRendering) {
		QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);
	}

	auto root = RootWrapper(configFilePath, shellId);
	QGuiApplication::setQuitOnLastWindowClosed(false);

	auto code = QGuiApplication::exec();
	delete app;
	return code;
}
