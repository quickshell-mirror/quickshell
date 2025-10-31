#pragma once

#include <qcontainerfwd.h>
#include <qdir.h>
#include <qhash.h>
#include <qloggingcategory.h>
#include <qvector.h>

#include "logcat.hpp"

QS_DECLARE_LOGGING_CATEGORY(logQmlScanner);

// expects canonical paths
class QmlScanner {
public:
	QmlScanner() = default;
	QmlScanner(const QDir& rootPath): rootPath(rootPath) {}

	void scanDir(const QDir& dir);
	void scanQmlRoot(const QString& path);

	QVector<QDir> scannedDirs;
	QVector<QString> scannedFiles;
	QHash<QString, QString> fileIntercepts;

private:
	QDir rootPath;

	bool scanQmlFile(const QString& path, bool& singleton, bool& internal);
	bool scanQmlJson(const QString& path);
	[[nodiscard]] static QPair<QString, QString> jsonToQml(const QJsonValue& value, int indent = 0);
};
