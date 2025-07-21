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

	QStringList getDesktopDirectories() const;

signals:
	void desktopEntriesChanged();

private slots:
	void onDirectoryChanged(const QString& path);
	void processChanges();

private:
	void initializeDesktopPaths();
	void startMonitoring();
	void scanAndWatch(const QString& dirPath);

	QFileSystemWatcher* watcher;
	QStringList desktopPaths;
	QTimer* debounceTimer;
	bool rescanPending = false;
};