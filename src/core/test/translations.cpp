#include "translations.hpp"

#include <qdir.h>
#include <qfile.h>
#include <qprocess.h>
#include <qtemporarydir.h>
#include <qtest.h>
#include <qtestcase.h>

void TestTranslations::catalogAndLanguageSwitch() {
	const QTemporaryDir directory;
	QVERIFY(directory.isValid());
	auto root = QDir(directory.path());
	QVERIFY(root.mkdir("i18n"));

	QVERIFY(QFile::copy(QFINDTESTDATA("translations/shell.qml"), root.filePath("shell.qml")));
	QVERIFY(QFile::copy("qml_ru.qm", root.filePath("i18n/qml_ru.qm")));

	auto environment = QProcessEnvironment::systemEnvironment();
	environment.insert("QT_QPA_PLATFORM", "offscreen");
	environment.insert("QT_QPA_PLATFORMTHEME", "generic");
	environment.insert("LC_ALL", "C.UTF-8");
	environment.insert("LANGUAGE", "ru_RU");
	environment.insert("QS_DISABLE_FILE_WATCHER", "1");
	environment.insert("XDG_RUNTIME_DIR", root.path());
	environment.insert("XDG_CACHE_HOME", root.filePath("cache"));
	environment.insert("XDG_STATE_HOME", root.filePath("state"));
	environment.insert("XDG_DATA_HOME", root.filePath("data"));
	environment.remove("WAYLAND_DISPLAY");

	QProcess shell;
	shell.setProcessEnvironment(environment);
	shell.setProcessChannelMode(QProcess::MergedChannels);
	shell.start(QS_BINARY, {"-p", root.filePath("shell.qml")});
	auto finished = shell.waitForFinished(10000);
	if (!finished) {
		shell.kill();
		shell.waitForFinished(3000);
	}
	auto output = shell.readAll();
	QVERIFY2(finished, output.constData());
	QCOMPARE(shell.exitStatus(), QProcess::NormalExit);
	QVERIFY2(shell.exitCode() == 0, output.constData());
}

QTEST_GUILESS_MAIN(TestTranslations);
