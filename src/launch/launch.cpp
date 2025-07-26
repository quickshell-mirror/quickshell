#include <qapplication.h>
#include <qcoreapplication.h>
#include <qcryptographichash.h>
#include <qdebug.h>
#include <qdir.h>
#include <qfile.h>
#include <qguiapplication.h>
#include <qhash.h>
#include <qlist.h>
#include <qlogging.h>
#include <qnamespace.h>
#include <qprocess.h>
#include <qqmldebug.h>
#include <qquickwindow.h>
#include <qstring.h>
#include <qtenvironmentvariables.h>
#include <qtextstream.h>
#include <unistd.h>

#include "../core/common.hpp"
#include "../core/instanceinfo.hpp"
#include "../core/logging.hpp"
#include "../core/paths.hpp"
#include "../core/plugin.hpp"
#include "../core/rootwrapper.hpp"
#include "../ipc/ipc.hpp"
#include "build.hpp"
#include "launch_p.hpp"

#if CRASH_REPORTER
#include "../crash/handler.hpp"
#endif

namespace qs::launch {

namespace {

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

} // namespace

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
		bool useSystemStyle = false;
		QString iconTheme = qEnvironmentVariable("QS_ICON_THEME");
		QHash<QString, QString> envOverrides;
		QString dataDir;
		QString stateDir;
	} pragmas;

	auto stream = QTextStream(&file);
	while (!stream.atEnd()) {
		auto line = stream.readLine().trimmed();
		if (line.startsWith("//@ pragma ")) {
			auto pragma = line.sliced(11).trimmed();

			if (pragma == "UseQApplication") pragmas.useQApplication = true;
			else if (pragma == "NativeTextRendering") pragmas.nativeTextRendering = true;
			else if (pragma == "IgnoreSystemSettings") pragmas.desktopSettingsAware = false;
			else if (pragma == "RespectSystemStyle") pragmas.useSystemStyle = true;
			else if (pragma.startsWith("IconTheme ")) pragmas.iconTheme = pragma.sliced(10);
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
			} else if (pragma.startsWith("DataDir ")) {
				pragmas.dataDir = pragma.sliced(8).trimmed();
			} else if (pragma.startsWith("StateDir ")) {
				pragmas.stateDir = pragma.sliced(9).trimmed();
			} else {
				qCritical() << "Unrecognized pragma" << pragma;
				return -1;
			}
		} else if (line.startsWith("import")) break;
	}

	file.close();

	if (!pragmas.iconTheme.isEmpty()) {
		QIcon::setThemeName(pragmas.iconTheme);
	}

	qInfo() << "Shell ID:" << shellId << "Path ID" << pathId;

	auto launchTime = qs::Common::LAUNCH_TIME.toSecsSinceEpoch();
	InstanceInfo::CURRENT = InstanceInfo {
	    .instanceId = base36Encode(getpid()) + base36Encode(launchTime),
	    .configPath = args.configPath,
	    .shellId = shellId,
	    .launchTime = qs::Common::LAUNCH_TIME,
	    .pid = getpid(),
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

	QsPaths::init(shellId, pathId, pragmas.dataDir, pragmas.stateDir);
	QsPaths::instance()->linkRunDir();
	QsPaths::instance()->linkPathDir();
	LogManager::initFs();

	Common::INITIAL_ENVIRONMENT = QProcessEnvironment::systemEnvironment();

	if (!pragmas.useSystemStyle) {
		qunsetenv("QT_STYLE_OVERRIDE");
		qputenv("QT_QUICK_CONTROLS_STYLE", "Fusion");
	}

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

	QGuiApplication::setDesktopFileName("org.quickshell");

	if (args.debugPort != -1) {
		QQmlDebuggingEnabler::enableDebugging(true);
		auto wait = args.waitForDebug ? QQmlDebuggingEnabler::WaitForClient
		                              : QQmlDebuggingEnabler::DoNotWaitForClient;
		QQmlDebuggingEnabler::startTcpDebugServer(args.debugPort, wait);
	}

	QsEnginePlugin::initPlugins();

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
