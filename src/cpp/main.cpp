#include <LayerShellQt/shell.h>
#include <qcommandlineoption.h>
#include <qcommandlineparser.h>
#include <qdir.h>
#include <qguiapplication.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlapplicationengine.h>
#include <qstandardpaths.h>
#include <qstring.h>

#include "shell.hpp"

int main(int argc, char** argv) {
	const auto app = QGuiApplication(argc, argv);
	QGuiApplication::setApplicationName("qtshell");
	QGuiApplication::setApplicationVersion("0.0.1");

	QCommandLineParser parser;
	parser.setApplicationDescription("Qt based desktop shell");
	parser.addHelpOption();
	parser.addVersionOption();

	auto configOption = QCommandLineOption({"c", "config"}, "Path to configuration file.", "path");
	parser.addOption(configOption);
	parser.process(app);

	QString configPath;
	if (parser.isSet(configOption)) {
		configPath = parser.value(configOption);
	} else {
		configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
		configPath = QDir(QDir(configPath).filePath("qtshell")).filePath("config.qml");
	}

	qInfo() << "config file path:" << configPath;

	if (!QFile(configPath).exists()) {
		qCritical() << "config file does not exist";
		return -1;
	}

	CONFIG_PATH = configPath;

	LayerShellQt::Shell::useLayerShell();

	auto engine = QQmlApplicationEngine();
	engine.load(configPath);

	if (engine.rootObjects().isEmpty()) {
		qCritical() << "failed to load config file";
		return -1;
	}

	auto* shellobj = qobject_cast<QtShell*>(engine.rootObjects().first());

	if (shellobj == nullptr) {
		qCritical() << "root item was not a QtShell";
		return -1;
	}

	return QGuiApplication::exec();
}
