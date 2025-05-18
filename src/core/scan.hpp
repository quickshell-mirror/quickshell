#pragma once

#include <qcontainerfwd.h>
#include <qdir.h>
#include <qhash.h>
#include <qloggingcategory.h>
#include <qvector.h>

Q_DECLARE_LOGGING_CATEGORY(logQmlScanner);

// expects canonical paths
class QmlScanner {
public:
	QmlScanner() = default;
	QmlScanner(const QDir& rootPath): rootPath(rootPath) {}

	void scanDir(const QString& path);
	// returns if the file has a singleton
	bool scanQmlFile(const QString& path);

	QVector<QString> scannedDirs;
	QVector<QString> scannedFiles;
	QHash<QString, QString> qmldirIntercepts;

private:
	QDir rootPath;
};
