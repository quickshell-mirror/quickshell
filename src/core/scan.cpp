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

	struct Entry {
		QString name;
		bool singleton = false;
		bool internal = false;
	};

	bool seenQmldir = false;
	auto entries = QVector<Entry>();

	for (auto& name: dir.entryList(QDir::Files | QDir::NoDotAndDotDot)) {
		if (name == "qmldir") {
			qCDebug(logQmlScanner
			) << "Found qmldir file, qmldir synthesization will be disabled for directory"
			  << path;
			seenQmldir = true;
		} else if (name.at(0).isUpper() && name.endsWith(".qml")) {
			auto& entry = entries.emplaceBack();

			if (this->scanQmlFile(dir.filePath(name), entry.singleton, entry.internal)) {
				entry.name = name;
			} else {
				entries.pop_back();
			}
		} else if (name.at(0).isUpper() && name.endsWith(".qml.json")) {
			if (this->scanQmlJson(dir.filePath(name))) {
				entries.push_back({
				    .name = name.first(name.length() - 5),
				    .singleton = true,
				});
			}
		}
	}

	if (!seenQmldir) {
		qCDebug(logQmlScanner) << "Synthesizing qmldir for directory" << path;

		QString qmldir;
		auto stream = QTextStream(&qmldir);

		// cant derive a module name if not in shell path
		if (path.startsWith(this->rootPath.path())) {
			auto end = path.sliced(this->rootPath.path().length());

			// verify we have a valid module name.
			for (auto& c: end) {
				if (c == '/') c = '.';
				else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
				         || c == '_')
				{
				} else {
					qCWarning(logQmlScanner) << "Module path contains invalid characters for a module name: "
					                         << path.sliced(this->rootPath.path().length());
					goto skipadd;
				}
			}

			stream << "module qs" << end << '\n';
		skipadd:;
		} else {
			qCWarning(logQmlScanner) << "Module path" << path << "is outside of the config folder.";
		}

		for (const auto& entry: entries) {
			if (entry.internal) stream << "internal ";
			if (entry.singleton) stream << "singleton ";
			stream << entry.name.sliced(0, entry.name.length() - 4) << " 1.0 " << entry.name << '\n';
		}

		qCDebug(logQmlScanner) << "Synthesized qmldir for" << path << qPrintable("\n" + qmldir);
		this->fileIntercepts.insert(QDir(path).filePath("qmldir"), qmldir);
	}
}

bool QmlScanner::scanQmlFile(const QString& path, bool& singleton, bool& internal) {
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

	while (!stream.atEnd()) {
		auto line = stream.readLine().trimmed();
		if (!singleton && line == "pragma Singleton") {
			singleton = true;
		} else if (!internal && line == "//@ pragma Internal") {
			internal = true;
		} else if (line.startsWith("import")) {
			// we dont care about "import qs" as we always load the root folder
			if (auto importCursor = line.indexOf(" qs."); importCursor != -1) {
				importCursor += 4;
				QString path;

				while (importCursor != line.length()) {
					auto c = line.at(importCursor);
					if (c == '.') c = '/';
					else if (c == ' ') break;
					else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
					         || c == '_')
					{
					} else {
						qCWarning(logQmlScanner) << "Import line contains invalid characters: " << line;
						goto next;
					}

					path.append(c);
					importCursor += 1;
				}

				imports.append(this->rootPath.filePath(path));
			} else if (auto startQuot = line.indexOf('"');
			           startQuot != -1 && line.length() >= startQuot + 3)
			{
				auto endQuot = line.indexOf('"', startQuot + 1);
				if (endQuot == -1) continue;

				auto name = line.sliced(startQuot + 1, endQuot - startQuot - 1);
				imports.push_back(name);
			}
		} else if (line.contains('{')) break;

	next:;
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

		auto pathInfo = QFileInfo(ipath);
		auto cpath = pathInfo.canonicalFilePath();

		if (cpath.isEmpty()) {
			qCWarning(logQmlScanner) << "Ignoring unresolvable import" << ipath << "from" << path;
			continue;
		}

		if (!pathInfo.isDir()) {
			qCDebug(logQmlScanner) << "Ignoring non-directory import" << ipath << "from" << path;
			continue;
		}

		if (import.endsWith(".js")) this->scannedFiles.push_back(cpath);
		else this->scanDir(cpath);
	}

	return true;
}

void QmlScanner::scanQmlRoot(const QString& path) {
	bool singleton = false;
	bool internal = false;
	this->scanQmlFile(path, singleton, internal);
}

bool QmlScanner::scanQmlJson(const QString& path) {
	qCDebug(logQmlScanner) << "Scanning qml.json file" << path;

	auto file = QFile(path);
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		qCWarning(logQmlScanner) << "Failed to open file" << path;
		return false;
	}

	auto data = file.readAll();

	// Importing this makes CI builds fail for some reason.
	QJsonParseError error; // NOLINT (misc-include-cleaner)
	auto json = QJsonDocument::fromJson(data, &error);

	if (error.error != QJsonParseError::NoError) {
		qCCritical(logQmlScanner).nospace()
		    << "Failed to parse qml.json file at " << path << ": " << error.errorString();
		return false;
	}

	const QString body =
	    "pragma Singleton\nimport QtQuick as Q\n\n" % QmlScanner::jsonToQml(json.object()).second;

	qCDebug(logQmlScanner) << "Synthesized qml file for" << path << qPrintable("\n" + body);

	this->fileIntercepts.insert(path.first(path.length() - 5), body);
	this->scannedFiles.push_back(path);
	return true;
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
