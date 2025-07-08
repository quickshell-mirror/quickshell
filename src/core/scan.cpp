#include "scan.hpp"
#include <cmath>

#include <qcontainerfwd.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qpair.h>
#include <qstring.h>
#include <qstringliteral.h>
#include <qtextstream.h>

#include "logcat.hpp"

QS_LOGGING_CATEGORY(logQmlScanner, "quickshell.qmlscanner", QtWarningMsg);

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
		} else if (entry.at(0).isUpper() && entry.endsWith(".qml.json")) {
			this->scanQmlJson(dir.filePath(entry));
			singletons.push_back(entry.first(entry.length() - 5));
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
		this->fileIntercepts.insert(QDir(path).filePath("qmldir"), qmldir);
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
		QString ipath;
		if (import.startsWith("root:")) {
			auto path = import.sliced(5);
			if (path.startsWith('/')) path = path.sliced(1);
			ipath = this->rootPath.filePath(path);
		} else {
			ipath = currentdir.filePath(import);
		}

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

void QmlScanner::scanQmlJson(const QString& path) {
	qCDebug(logQmlScanner) << "Scanning qml.json file" << path;

	auto file = QFile(path);
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		qCWarning(logQmlScanner) << "Failed to open file" << path;
		return;
	}

	auto data = file.readAll();

	// Importing this makes CI builds fail for some reason.
	QJsonParseError error; // NOLINT (misc-include-cleaner)
	auto json = QJsonDocument::fromJson(data, &error);

	if (error.error != QJsonParseError::NoError) {
		qCCritical(logQmlScanner).nospace()
		    << "Failed to parse qml.json file at " << path << ": " << error.errorString();
		return;
	}

	const QString body =
	    "pragma Singleton\nimport QtQuick as Q\n\n" % QmlScanner::jsonToQml(json.object()).second;

	qCDebug(logQmlScanner) << "Synthesized qml file for" << path << qPrintable("\n" + body);

	this->fileIntercepts.insert(path.first(path.length() - 5), body);
	this->scannedFiles.push_back(path);
}

QPair<QString, QString> QmlScanner::jsonToQml(const QJsonValue& value, int indent) {
	if (value.isObject()) {
		const auto& object = value.toObject();

		auto valIter = object.constBegin();

		QString accum = "Q.QtObject {\n";
		for (const auto& key: object.keys()) {
			const auto& val = *valIter++;
			auto [type, repr] = QmlScanner::jsonToQml(val, indent + 2);
			accum += QString(' ').repeated(indent + 2) % "readonly property " % type % ' ' % key % ": "
			       % repr % ";\n";
		}

		accum += QString(' ').repeated(indent) % '}';
		return qMakePair(QStringLiteral("Q.QtObject"), accum);
	} else if (value.isArray()) {
		return qMakePair(
		    QStringLiteral("var"),
		    QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact)
		);
	} else if (value.isString()) {
		const auto& str = value.toString();

		if (str.startsWith('#') && (str.length() == 4 || str.length() == 7 || str.length() == 9)) {
			for (auto c: str.sliced(1)) {
				if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
					goto noncolor;
				}
			}

			return qMakePair(QStringLiteral("Q.color"), '"' % str % '"');
		}

	noncolor:
		return qMakePair(QStringLiteral("string"), '"' % QString(str).replace("\"", "\\\"") % '"');
	} else if (value.isDouble()) {
		auto num = value.toDouble();
		double whole = 0;
		if (std::modf(num, &whole) == 0.0) {
			return qMakePair(QStringLiteral("int"), QString::number(static_cast<int>(whole)));
		} else {
			return qMakePair(QStringLiteral("real"), QString::number(num));
		}
	} else if (value.isBool()) {
		return qMakePair(QStringLiteral("bool"), value.toBool() ? "true" : "false");
	} else {
		return qMakePair(QStringLiteral("var"), "null");
	}
}
