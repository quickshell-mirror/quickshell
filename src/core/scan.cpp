#include "scan.hpp"

#include <qcontainerfwd.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qstring.h>
#include <qtextstream.h>

Q_LOGGING_CATEGORY(logQmlScanner, "quickshell.qmlscanner", QtWarningMsg);

void QmlScanner::scanDir(const QString& path) {
	if (this->scannedDirs.contains(path)) return;
	this->scannedDirs.push_back(path);

	qCDebug(logQmlScanner) << "Scanning directory" << path;
	auto dir = QDir(path);

	bool seenQmldir = false;
	auto singletons = QVector<QString>();
	auto entries = QVector<QString>();
	for (auto& entry: dir.entryList(QDir::Files | QDir::NoDotAndDotDot)) {
		if (entry == "qmldir") {
			qCDebug(logQmlScanner
			) << "Found qmldir file, qmldir synthesization will be disabled for directory"
			  << path;
			seenQmldir = true;
		} else if (entry.at(0).isUpper() && entry.endsWith(".qml")) {
			if (this->scanQmlFile(dir.filePath(entry))) {
				singletons.push_back(entry);
			} else {
				entries.push_back(entry);
			}
		}
	}

	// Due to the qsintercept:// protocol a qmldir is always required, even without singletons.
	if (!seenQmldir) {
		qCDebug(logQmlScanner) << "Synthesizing qmldir for directory" << path << "singletons"
		                       << singletons;

		QString qmldir;
		auto stream = QTextStream(&qmldir);

		for (auto& singleton: singletons) {
			stream << "singleton " << singleton.sliced(0, singleton.length() - 4) << " 1.0 " << singleton
			       << "\n";
		}

		for (auto& entry: entries) {
			stream << entry.sliced(0, entry.length() - 4) << " 1.0 " << entry << "\n";
		}

		qCDebug(logQmlScanner) << "Synthesized qmldir for" << path << qPrintable("\n" + qmldir);
		this->qmldirIntercepts.insert(QDir(path).filePath("qmldir"), qmldir);
	}
}

bool QmlScanner::scanQmlFile(const QString& path) {
	if (this->scannedFiles.contains(path)) return false;
	this->scannedFiles.push_back(path);

	qCDebug(logQmlScanner) << "Scanning qml file" << path;

	auto file = QFile(path);
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		qCWarning(logQmlScanner) << "Failed to open file" << path;
		return false;
	}

	auto stream = QTextStream(&file);
	auto imports = QVector<QString>();

	bool singleton = false;

	while (!stream.atEnd()) {
		auto line = stream.readLine().trimmed();
		if (!singleton && line == "pragma Singleton") {
			qCDebug(logQmlScanner) << "Discovered singleton" << path;
			singleton = true;
		} else if (line.startsWith("import")) {

			auto startQuot = line.indexOf('"');
			if (startQuot == -1 || line.length() < startQuot + 3) continue;
			auto endQuot = line.indexOf('"', startQuot + 1);
			if (endQuot == -1) continue;

			auto name = line.sliced(startQuot + 1, endQuot - startQuot - 1);
			imports.push_back(name);
		} else if (line.contains('{')) break;
	}

	file.close();

	if (logQmlScanner().isDebugEnabled() && !imports.isEmpty()) {
		qCDebug(logQmlScanner) << "Found imports" << imports;
	}

	auto currentdir = QDir(QFileInfo(path).canonicalPath());

	// the root can never be a singleton so it dosent matter if we skip it
	this->scanDir(currentdir.path());

	for (auto& import: imports) {
		auto ipath = currentdir.filePath(import);
		auto cpath = QFileInfo(ipath).canonicalFilePath();

		if (cpath.isEmpty()) {
			qCWarning(logQmlScanner) << "Ignoring unresolvable import" << ipath << "from" << path;
			continue;
		}

		if (import.endsWith(".js")) this->scannedFiles.push_back(cpath);
		else this->scanDir(cpath);
	}

	return singleton;
}
