#include <qcommandlineoption.h>
#include <qcommandlineparser.h>
#include <qdir.h>
#include <qguiapplication.h>
#include <qlogging.h>
#include <qobject.h>
#include <qquickwindow.h>
#include <qstandardpaths.h>
#include <qstring.h>

#include "rootwrapper.hpp"

int main(int argc, char** argv) {
	const auto app = QGuiApplication(argc, argv);
	QGuiApplication::setApplicationName("quickshell");
	QGuiApplication::setApplicationVersion("0.0.1");

	QCommandLineParser parser;
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
		configPath = QDir(QDir(configPath).filePath("quickshell")).filePath("shell.qml");
	}

	qInfo() << "config file path:" << configPath;

	if (!QFile(configPath).exists()) {
		qCritical() << "config file does not exist";
		return -1;
	}

	// Base window transparency appears to be additive.
	// Use a fully transparent window with a colored rect.
	QQuickWindow::setDefaultAlphaBuffer(true);

	auto root = RootWrapper(configPath);

	return QGuiApplication::exec();
}
