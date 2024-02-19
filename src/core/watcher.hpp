#pragma once

#include <qdir.h>
#include <qfilesystemwatcher.h>
#include <qobject.h>

class FiletreeWatcher: public QObject {
	Q_OBJECT;

public:
	explicit FiletreeWatcher(QObject* parent = nullptr);

	void addPath(const QString& path);

signals:
	void fileChanged(const QString& path);

private slots:
	void onDirectoryChanged(const QString& path);
	void onFileChanged(const QString& path);

private:
	QFileSystemWatcher watcher;
};
