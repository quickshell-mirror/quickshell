#pragma once

#include <qfilesystemwatcher.h>
#include <qobject.h>
#include <qstringlist.h>
#include <qtimer.h>

class DesktopEntryMonitor: public QObject {
	Q_OBJECT

public:
	explicit DesktopEntryMonitor(QObject* parent = nullptr);
	~DesktopEntryMonitor() = default;

signals:
	void desktopEntriesChanged();

private slots:
	void onDirectoryChanged(const QString& path);
	void processChanges();

private:
	void startMonitoring();
	void scanAndWatch(const QString& dirPath);

	QFileSystemWatcher watcher;
	QTimer debounceTimer;
};