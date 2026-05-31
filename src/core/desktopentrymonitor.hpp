#pragma once

#include <qfilesystemwatcher.h>
#include <qobject.h>
#include <qstringlist.h>
#include <qtimer.h>

class DesktopEntryMonitor: public QObject {
	Q_OBJECT

public:
	explicit DesktopEntryMonitor(QObject* parent = nullptr);
	~DesktopEntryMonitor() override = default;
	DesktopEntryMonitor(const DesktopEntryMonitor&) = delete;
	DesktopEntryMonitor& operator=(const DesktopEntryMonitor&) = delete;
	DesktopEntryMonitor(DesktopEntryMonitor&&) = delete;
	DesktopEntryMonitor& operator=(DesktopEntryMonitor&&) = delete;

signals:
	void desktopEntriesChanged();

private slots:
	void onWatchedPathChanged(const QString& path);
	void processChanges();

private:
	void updateWatchedPaths();
	void scanAndWatch(const QString& dirPath);

	QFileSystemWatcher watcher;
	QTimer debounceTimer;
};
