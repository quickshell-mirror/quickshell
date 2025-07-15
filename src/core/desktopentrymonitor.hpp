#pragma once

#include <qdatetime.h>
#include <qfilesystemwatcher.h>
#include <qhash.h>
#include <qobject.h>
#include <qset.h>
#include <qstringlist.h>
#include <qtimer.h>

class DesktopEntryMonitor: public QObject {
	Q_OBJECT

public:
	enum class ChangeEvent { Added, Modified, Removed };

	explicit DesktopEntryMonitor(QObject* parent = nullptr);
	~DesktopEntryMonitor() = default;

signals:
	void desktopEntriesChanged(const QHash<QString, ChangeEvent>& changes);

private slots:
	void onDirectoryChanged(const QString& path);
	void onFileChanged(const QString& path);
	void processChanges();

private:
	void initializeDesktopPaths();
	void startMonitoring();
	void addDirectoryHierarchy(const QString& dirPath);
	void scanAndWatch(const QString& dirPath);
	void queueChange(ChangeEvent event, const QString& path);

	QFileSystemWatcher* watcher;
	QStringList desktopPaths;
	QHash<QString, QDateTime> fileTimestamps;
	QSet<QString> watchedFiles;
	QTimer* debounceTimer;
	QHash<QString, ChangeEvent> pendingChanges;
};